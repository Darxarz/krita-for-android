package org.qtproject.qt5.android;

import android.app.Activity;
import android.app.Service;
import android.content.pm.PackageManager;
import android.os.Build;
import android.view.KeyEvent;
import android.view.MotionEvent;

public final class QtNative {
    private static Activity probeActivity;
    private static ClassLoader probeClassLoader;

    private QtNative() {
    }

    public static void setActivityForProbe(Activity activity) {
        probeActivity = activity;
        probeClassLoader = activity == null ? QtNative.class.getClassLoader() : activity.getClassLoader();
    }

    public static Activity activity() {
        return probeActivity;
    }

    public static Service service() {
        return null;
    }

    public static ClassLoader classLoader() {
        return probeClassLoader == null ? QtNative.class.getClassLoader() : probeClassLoader;
    }

    private static native void runPendingCppRunnables();
    private static native boolean dispatchGenericMotionEvent(MotionEvent event);
    private static native boolean dispatchKeyEvent(KeyEvent event);
    private static native void setNativeActivity(Activity activity);
    private static native void setNativeService(Service service);
    private static native void sendRequestPermissionsResult(
            int requestCode, String[] permissions, int[] grantResults);

    public static void runPendingCppRunnablesOnAndroidThread() {
        Activity activity = probeActivity;
        if (activity == null) {
            runPendingCppRunnables();
            return;
        }

        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                runPendingCppRunnables();
            }
        });
    }

    public static void hideSplashScreen(int duration) {
    }

    public static int checkSelfPermission(String permission) {
        Activity activity = probeActivity;
        if (activity == null || Build.VERSION.SDK_INT < 23) {
            return PackageManager.PERMISSION_GRANTED;
        }
        return activity.checkSelfPermission(permission);
    }

    public static boolean shouldShowRequestPermissionRationale(String permission) {
        Activity activity = probeActivity;
        return activity != null
                && Build.VERSION.SDK_INT >= 23
                && activity.shouldShowRequestPermissionRationale(permission);
    }

    public static void requestPermissions(String[] permissions, int requestCode) {
        Activity activity = probeActivity;
        if (activity != null && Build.VERSION.SDK_INT >= 23) {
            activity.requestPermissions(permissions, requestCode);
            return;
        }

        int[] grantResults = new int[permissions == null ? 0 : permissions.length];
        for (int index = 0; index < grantResults.length; index++) {
            grantResults[index] = PackageManager.PERMISSION_GRANTED;
        }
        sendRequestPermissionsResult(requestCode, permissions, grantResults);
    }
}
