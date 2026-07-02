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
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public final class MainActivity extends Activity {
    private static final String TAG = "KritaPyRuntimeProbe";
    private static final String SCREEN_TITLE = "Krita Probe Manual v9";

    private static native String runInitProbe(String runtimeRoot);
    private static native String runImportProbe(String runtimeRoot);
    private static native String runImportOneProbe(String runtimeRoot, String moduleName);
    private static native String runChildImportOneProbe(String runtimeRoot, String moduleName);
    private static native String runChildDlopenPyKritaProbe(String runtimeRoot);

    private TextView statusView;
    private TextView logView;
    private volatile boolean pythonLibraryLoaded;
    private volatile boolean initProbeLibraryLoaded;
    private volatile boolean launcherLibraryLoaded;
    private volatile boolean taskRunning;
    private volatile String lastShortStatus = "not started";
    private final StringBuilder logBuffer = new StringBuilder();
    private long lastProgressUpdateMs;
    private int uiClickCount;

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

        root.addView(makeUiTestButton());

        root.addView(makeButton("1a. Load libpython only", new Task() {
            @Override
            public void run() {
                loadPythonLibraryIfNeeded();
            }
        }));

        root.addView(makeButton("1b. Load init probe", new Task() {
            @Override
            public void run() {
                loadInitProbeLibraryIfNeeded();
            }
        }));

        root.addView(makeButton("1c. Load launcher", new Task() {
            @Override
            public void run() {
                loadLauncherLibraryIfNeeded();
            }
        }));

        root.addView(makeButton("1. Load all native libraries", new Task() {
            @Override
            public void run() {
                loadAllNativeLibrariesIfNeeded();
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

        root.addView(makeImportOneButton("4a. Import sys", "sys"));
        root.addView(makeImportOneButton("4b. Import PyQt5.QtCore", "PyQt5.QtCore"));
        root.addView(makeChildImportOneButton("4b1. Child import PyQt5.QtGui", "PyQt5.QtGui"));
        root.addView(makeChildImportOneButton("4b2. Child import PyQt5.QtWidgets", "PyQt5.QtWidgets"));
        root.addView(makeChildImportOneButton("4b3. Child import PyQt5.QtXml", "PyQt5.QtXml"));
        root.addView(makeChildDlopenPyKritaButton());
        root.addView(makeChildImportOneButton("4c1. Child import PyKrita.krita", "PyKrita.krita"));
        root.addView(makeImportOneButton("4cZ. Import PyKrita.krita crash test", "PyKrita.krita"));
        root.addView(makeImportOneButton("4d. Import krita", "krita"));

        root.addView(makeButton("4z. Import all Python modules crash test", new Task() {
            @Override
            public void run() throws IOException {
                loadNativeLibrariesIfNeeded();
                requirePayload();
                showStep("Running combined Python import probe...");
                showResult(runImportProbe(runtimeRoot().getAbsolutePath()));
            }
        }));

        root.addView(makeButton("Run setup sequence", new Task() {
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

        logView = new TextView(this);
        logView.setTextColor(Color.rgb(30, 30, 30));
        logView.setBackgroundColor(Color.rgb(245, 245, 245));
        logView.setGravity(Gravity.START | Gravity.TOP);
        logView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        logView.setPadding(16, 16, 16, 16);
        LinearLayout.LayoutParams logParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
        logParams.setMargins(0, 20, 0, 0);
        root.addView(logView, logParams);

        scrollView.addView(root);
        setContentView(scrollView);

        showStep("Idle. No heavy task is running.\n\n"
                + "First press 'UI click test'. It does not load Python or copy files.");
    }

    private Button makeUiTestButton() {
        final Button button = new Button(this);
        button.setAllCaps(false);
        button.setText("UI click test");
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
        params.setMargins(0, 12, 0, 0);
        button.setLayoutParams(params);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                uiClickCount++;
                String message = SCREEN_TITLE + "\n\nUI click received: " + uiClickCount
                        + "\nNo native libraries loaded. No payload copied.";
                showResult(message);
                button.setText("UI click test: " + uiClickCount);
                Toast.makeText(MainActivity.this, "UI click " + uiClickCount, Toast.LENGTH_SHORT).show();
            }
        });
        return button;
    }

    private Button makeImportOneButton(String label, final String moduleName) {
        return makeButton(label, new Task() {
            @Override
            public void run() throws IOException {
                loadNativeLibrariesIfNeeded();
                requirePayload();
                showStep("Running single import probe: " + moduleName);
                showResult(runImportOneProbe(runtimeRoot().getAbsolutePath(), moduleName));
            }
        });
    }

    private Button makeChildImportOneButton(String label, final String moduleName) {
        return makeButton(label, new Task() {
            @Override
            public void run() throws IOException {
                loadNativeLibrariesIfNeeded();
                requirePayload();
                showStep("Running child import probe: " + moduleName);
                showResult(runChildImportOneProbe(runtimeRoot().getAbsolutePath(), moduleName));
            }
        });
    }

    private Button makeChildDlopenPyKritaButton() {
        return makeButton("4c0. Child dlopen PyKrita.krita", new Task() {
            @Override
            public void run() throws IOException {
                loadNativeLibrariesIfNeeded();
                requirePayload();
                showStep("Running child dlopen probe: PyKrita.krita");
                showResult(runChildDlopenPyKritaProbe(runtimeRoot().getAbsolutePath()));
            }
        });
    }

    private Button makeButton(String label, final Task task) {
        final Button button = new Button(this);
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
                String clicked = SCREEN_TITLE + "\n\nClicked: " + label
                        + "\nTask will start in 750 ms.";
                showResult(clicked);
                button.setText(label + "\nRUNNING...");
                button.setEnabled(false);
                Toast.makeText(MainActivity.this, "Clicked: " + label, Toast.LENGTH_SHORT).show();

                button.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        startTask(label, task, button);
                    }
                }, 750);
            }
        });
        return button;
    }

    private void startTask(final String label, final Task task, final Button button) {
        if (taskRunning) {
            showStep("A task is already running. Wait for it to finish.");
            button.setText(label);
            button.setEnabled(true);
            return;
        }

        taskRunning = true;
        new Thread(new Runnable() {
            @Override
            public void run() {
                final String[] failure = new String[1];
                try {
                    task.run();
                } catch (Throwable error) {
                    failure[0] = error.toString();
                    Log.e(TAG, "Runtime probe task failed", error);
                    showResult("FAILED: " + error);
                } finally {
                    taskRunning = false;
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (failure[0] == null) {
                                button.setText(label + "\nDONE: " + lastShortStatus);
                            } else {
                                button.setText(label + "\nFAILED: " + shorten(failure[0]));
                            }
                            button.setEnabled(true);
                        }
                    });
                }
            }
        }, "krita-python-runtime-probe").start();
    }

    private synchronized void loadNativeLibrariesIfNeeded() {
        loadAllNativeLibrariesIfNeeded();
    }

    private synchronized void loadAllNativeLibrariesIfNeeded() {
        loadPythonLibraryIfNeeded();
        loadInitProbeLibraryIfNeeded();
        loadLauncherLibraryIfNeeded();
        showStep("All native libraries loaded OK.");
    }

    private synchronized void loadPythonLibraryIfNeeded() {
        if (pythonLibraryLoaded) {
            showStep("libpython3.14.so already loaded.");
            return;
        }

        loadNativeLibrary("libpython3.14.so");
        pythonLibraryLoaded = true;
    }

    private synchronized void loadInitProbeLibraryIfNeeded() {
        loadPythonLibraryIfNeeded();
        if (initProbeLibraryLoaded) {
            showStep("libkrita_python_runtime_init_probe.so already loaded.");
            return;
        }

        loadNativeLibrary("libkrita_python_runtime_init_probe.so");
        initProbeLibraryLoaded = true;
    }

    private synchronized void loadLauncherLibraryIfNeeded() {
        loadInitProbeLibraryIfNeeded();
        if (launcherLibraryLoaded) {
            showStep("libkrita_python_runtime_launcher.so already loaded.");
            return;
        }

        loadNativeLibrary("libkrita_python_runtime_launcher.so");
        launcherLibraryLoaded = true;
    }

    private void loadNativeLibrary(String fileName) {
        File library = new File(getApplicationInfo().nativeLibraryDir, fileName);
        showStep("Loading " + fileName + "\n" + library.getAbsolutePath());
        sleepBeforeNativeLoad();
        System.load(library.getAbsolutePath());
        showStep("Loaded " + fileName + " OK.");
    }

    private void sleepBeforeNativeLoad() {
        try {
            Thread.sleep(300);
        } catch (InterruptedException error) {
            Thread.currentThread().interrupt();
        }
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
        lastShortStatus = shorten(stripScreenTitle(message));
        Log.i(TAG, message);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                statusView.setText(message);
                appendLogLine(lastShortStatus);
            }
        });
    }

    private void appendLogLine(String line) {
        if (logView == null) {
            return;
        }

        logBuffer.append(line).append('\n');
        while (logBuffer.length() > 3000) {
            int newline = logBuffer.indexOf("\n");
            if (newline < 0) {
                logBuffer.setLength(0);
                break;
            }
            logBuffer.delete(0, newline + 1);
        }

        logView.setText("Persistent log:\n" + logBuffer);
    }

    private static String stripScreenTitle(String message) {
        if (message.startsWith(SCREEN_TITLE)) {
            String stripped = message.substring(SCREEN_TITLE.length()).trim();
            return stripped.length() == 0 ? message : stripped;
        }
        return message;
    }

    private static String shorten(String value) {
        String normalized = value.replace('\n', ' ').replace('\r', ' ').trim();
        return normalized.length() <= 90 ? normalized : normalized.substring(0, 87) + "...";
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
