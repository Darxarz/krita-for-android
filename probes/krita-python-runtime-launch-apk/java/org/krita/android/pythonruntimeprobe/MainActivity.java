package org.krita.android.pythonruntimeprobe;

import android.app.Activity;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public final class MainActivity extends Activity {
    private static final String TAG = "KritaPyRuntimeProbe";
    private static final String SCREEN_TITLE = "Krita Probe Manual v3";

    private static native String runInitProbe(String runtimeRoot);

    private TextView statusView;
    private volatile boolean nativeLibrariesLoaded;
    private volatile boolean taskRunning;
    private long lastProgressUpdateMs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle(SCREEN_TITLE);

        ScrollView scrollView = new ScrollView(this);
        scrollView.setFillViewport(true);
        scrollView.setBackgroundColor(Color.WHITE);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(32, 32, 32, 32);
        root.setBackgroundColor(Color.WHITE);

        statusView = new TextView(this);
        statusView.setTextColor(Color.rgb(20, 20, 20));
        statusView.setBackgroundColor(Color.WHITE);
        statusView.setGravity(Gravity.START | Gravity.TOP);
        statusView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        statusView.setMinLines(8);
        root.addView(statusView, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

        root.addView(makeButton("1. Load native libraries", new Task() {
            @Override
            public void run() {
                loadNativeLibrariesIfNeeded();
                showStep("Native libraries loaded.");
            }
        }));

        root.addView(makeButton("2. Copy Python payload", new Task() {
            @Override
            public void run() throws IOException {
                copyPayloadIfNeeded(false);
            }
        }));

        root.addView(makeButton("3. Run PyConfig init", new Task() {
            @Override
            public void run() throws IOException {
                loadNativeLibrariesIfNeeded();
                requirePayload();
                showStep("Running PyConfig init probe...");
                showResult(runInitProbe(runtimeRoot().getAbsolutePath()));
            }
        }));

        root.addView(makeButton("Run full sequence", new Task() {
            @Override
            public void run() throws IOException {
                loadNativeLibrariesIfNeeded();
                copyPayloadIfNeeded(false);
                showStep("Running PyConfig init probe...");
                showResult(runInitProbe(runtimeRoot().getAbsolutePath()));
            }
        }));

        root.addView(makeButton("Reset copied payload", new Task() {
            @Override
            public void run() throws IOException {
                showStep("Deleting app-private Python payload...");
                deleteTree(runtimeRoot());
                showStep("Payload deleted. No heavy task is running.");
            }
        }));

        scrollView.addView(root);
        setContentView(scrollView);

        showStep("Idle. No heavy task is running.\n\n"
                + "Use the buttons one by one. If this text is visible, Activity.onCreate() works.");
    }

    private Button makeButton(String label, final Task task) {
        Button button = new Button(this);
        button.setAllCaps(false);
        button.setText(label);
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
        params.setMargins(0, 12, 0, 0);
        button.setLayoutParams(params);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                startTask(task);
            }
        });
        return button;
    }

    private void startTask(final Task task) {
        if (taskRunning) {
            showStep("A task is already running. Wait for it to finish.");
            return;
        }

        taskRunning = true;
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    task.run();
                } catch (Throwable error) {
                    Log.e(TAG, "Runtime probe task failed", error);
                    showResult("FAILED: " + error);
                } finally {
                    taskRunning = false;
                }
            }
        }, "krita-python-runtime-probe").start();
    }

    private synchronized void loadNativeLibrariesIfNeeded() {
        if (nativeLibrariesLoaded) {
            showStep("Native libraries already loaded.");
            return;
        }

        loadNativeLibrary("libpython3.14.so");
        loadNativeLibrary("libkrita_python_runtime_init_probe.so");
        loadNativeLibrary("libkrita_python_runtime_launcher.so");
        nativeLibrariesLoaded = true;
    }

    private void loadNativeLibrary(String fileName) {
        File library = new File(getApplicationInfo().nativeLibraryDir, fileName);
        showStep("Loading " + fileName + "\n" + library.getAbsolutePath());
        System.load(library.getAbsolutePath());
    }

    private void copyPayloadIfNeeded(boolean force) throws IOException {
        File runtimeRoot = runtimeRoot();
        File pythonAssetsRoot = pythonAssetsRoot();
        File sentinel = payloadSentinel();

        if (force || !sentinel.isFile()) {
            showStep("Preparing private runtime directory...");
            deleteTree(runtimeRoot);

            lastProgressUpdateMs = 0;
            showStep("Copying Python payload from APK assets. This is the heavy step.");
            copyAssetTree(getAssets(), "python", pythonAssetsRoot, new CopyStats());
            if (!sentinel.createNewFile()) {
                throw new IOException("Could not write payload sentinel");
            }
            showStep("Python payload copied.");
            return;
        }

        showStep("Python payload already copied. Reusing app-private storage.");
    }

    private void requirePayload() throws IOException {
        if (!payloadSentinel().isFile()) {
            throw new IOException("Python payload is not copied yet. Press 'Copy Python payload' first.");
        }
    }

    private File runtimeRoot() {
        return new File(getFilesDir(), "krita-python-runtime");
    }

    private File pythonAssetsRoot() {
        return new File(runtimeRoot(), "assets/python");
    }

    private File payloadSentinel() {
        return new File(pythonAssetsRoot(), ".payload_complete");
    }

    private void showStep(String message) {
        showResult(SCREEN_TITLE + "\n\n" + message);
    }

    private void showResult(final String message) {
        Log.i(TAG, message);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                statusView.setText(message);
            }
        });
    }

    private void showCopyProgress(String assetPath, CopyStats stats) {
        long now = System.currentTimeMillis();
        if (now - lastProgressUpdateMs < 500) {
            return;
        }

        lastProgressUpdateMs = now;
        showStep("Copying Python payload...\nfiles=" + stats.files
                + "\nbytes=" + stats.bytes
                + "\ncurrent=" + assetPath);
    }

    private void copyAssetTree(AssetManager assets, String assetPath, File target, CopyStats stats) throws IOException {
        String[] children = assets.list(assetPath);
        if (children != null && children.length > 0) {
            if (!target.isDirectory() && !target.mkdirs()) {
                throw new IOException("Could not create directory " + target);
            }
            for (String child : children) {
                copyAssetTree(assets, assetPath + "/" + child, new File(target, child), stats);
            }
            return;
        }

        File parent = target.getParentFile();
        if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
            throw new IOException("Could not create directory " + parent);
        }

        byte[] buffer = new byte[64 * 1024];
        try (InputStream input = assets.open(assetPath);
             OutputStream output = new FileOutputStream(target)) {
            int count;
            while ((count = input.read(buffer)) != -1) {
                output.write(buffer, 0, count);
                stats.bytes += count;
            }
        }

        stats.files++;
        showCopyProgress(assetPath, stats);
    }

    private static void deleteTree(File path) throws IOException {
        if (!path.exists()) {
            return;
        }

        File[] children = path.listFiles();
        if (children != null) {
            for (File child : children) {
                deleteTree(child);
            }
        }

        if (!path.delete()) {
            throw new IOException("Could not delete " + path);
        }
    }

    private interface Task {
        void run() throws Exception;
    }

    private static final class CopyStats {
        long files;
        long bytes;
    }
}
