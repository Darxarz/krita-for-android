package org.krita.android.pythonruntimeprobe;

import android.app.Activity;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public final class MainActivity extends Activity {
    private static final String TAG = "KritaPyRuntimeProbe";

    private static native String runInitProbe(String runtimeRoot);

    private TextView statusView;
    private long lastProgressUpdateMs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        statusView = new TextView(this);
        statusView.setText("Krita Python Runtime Probe\n\nStarting UI...");
        statusView.setTextColor(Color.rgb(20, 20, 20));
        statusView.setBackgroundColor(Color.WHITE);
        statusView.setGravity(Gravity.START | Gravity.TOP);
        statusView.setPadding(32, 32, 32, 32);
        statusView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        setContentView(statusView);

        statusView.postDelayed(new Runnable() {
            @Override
            public void run() {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        runProbe();
                    }
                }, "krita-python-runtime-probe").start();
            }
        }, 300);
    }

    private void runProbe() {
        final File runtimeRoot = new File(getFilesDir(), "krita-python-runtime");
        final File pythonAssetsRoot = new File(runtimeRoot, "assets/python");
        final File sentinel = new File(pythonAssetsRoot, ".payload_complete");

        try {
            showStep("Loading native libraries...");
            loadNativeLibrary("libpython3.14.so");
            loadNativeLibrary("libkrita_python_runtime_init_probe.so");
            loadNativeLibrary("libkrita_python_runtime_launcher.so");

            if (!sentinel.isFile()) {
                showStep("Preparing private runtime directory...");
                deleteTree(runtimeRoot);

                showStep("Copying Python payload from APK assets. This can take a while on first launch...");
                copyAssetTree(getAssets(), "python", pythonAssetsRoot, new CopyStats());
                if (!sentinel.createNewFile()) {
                    throw new IOException("Could not write payload sentinel");
                }
            } else {
                showStep("Python payload already copied. Reusing app-private storage...");
            }

            showStep("Running PyConfig init probe...");
            showResult(runInitProbe(runtimeRoot.getAbsolutePath()));
        } catch (Throwable error) {
            Log.e(TAG, "Runtime probe failed", error);
            showResult("FAILED: " + error);
        }
    }

    private void loadNativeLibrary(String fileName) {
        File library = new File(getApplicationInfo().nativeLibraryDir, fileName);
        showStep("Loading " + fileName + "\n" + library.getAbsolutePath());
        System.load(library.getAbsolutePath());
    }

    private void showStep(String message) {
        showResult("Krita Python Runtime Probe\n\n" + message);
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

    private static final class CopyStats {
        long files;
        long bytes;
    }
}
