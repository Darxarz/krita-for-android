package org.krita.android.pythonruntimeprobe;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public final class MainActivity extends Activity {
    private static final String TAG = "KritaPyRuntimeProbe";
    private static String libraryLoadError;

    static {
        try {
            System.loadLibrary("python3.14");
            System.loadLibrary("krita_python_runtime_init_probe");
            System.loadLibrary("krita_python_runtime_launcher");
        } catch (UnsatisfiedLinkError error) {
            libraryLoadError = error.toString();
            Log.e(TAG, "Native library load failed", error);
        }
    }

    private static native String runInitProbe(String runtimeRoot);

    private TextView statusView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        statusView = new TextView(this);
        statusView.setText("Preparing Krita Python runtime probe...");
        statusView.setPadding(32, 32, 32, 32);
        setContentView(statusView);

        if (libraryLoadError != null) {
            showResult("FAILED: " + libraryLoadError);
            return;
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                runProbe();
            }
        }, "krita-python-runtime-probe").start();
    }

    private void runProbe() {
        final File runtimeRoot = new File(getFilesDir(), "krita-python-runtime");
        final File pythonAssetsRoot = new File(runtimeRoot, "assets/python");
        final File sentinel = new File(pythonAssetsRoot, ".payload_complete");

        try {
            if (!sentinel.isFile()) {
                deleteTree(runtimeRoot);
                copyAssetTree(getAssets(), "python", pythonAssetsRoot);
                if (!sentinel.createNewFile()) {
                    throw new IOException("Could not write payload sentinel");
                }
            }

            showResult(runInitProbe(runtimeRoot.getAbsolutePath()));
        } catch (Throwable error) {
            Log.e(TAG, "Runtime probe failed", error);
            showResult("FAILED: " + error);
        }
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

    private static void copyAssetTree(AssetManager assets, String assetPath, File target) throws IOException {
        String[] children = assets.list(assetPath);
        if (children != null && children.length > 0) {
            if (!target.isDirectory() && !target.mkdirs()) {
                throw new IOException("Could not create directory " + target);
            }
            for (String child : children) {
                copyAssetTree(assets, assetPath + "/" + child, new File(target, child));
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
            }
        }
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
}
