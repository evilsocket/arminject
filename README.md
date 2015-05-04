ARM Inject
===

An application to dynamically inject a shared object into a running process on
ARM architectures and hook API calls.

Read more on [Dynamically inject a shared library into a running process on Android/ARM](http://www.evilsocket.net/2015/05/01/dynamically-inject-a-shared-library-into-a-running-process-on-androidarm/).

## How to Test

In order to test this, you'll need the Android NDK installed and a device connected to your USB port,
then simply run:

    make test

This will launch a new Chrome browser instance and inject libhook into it. Once injected
the library will hook the **open** function and print every call to it to the logcat.

    @ Attaching to process com.android.chrome ...
    @ Injecting library /data/local/tmp/libhook.so into process 8511.
    @ Calling dlopen in target process ...
    @ dlopen returned 0xb5202dc4

    I/LIBHOOK ( 8511): [8511] open('/data/data/com.android.chrome/app_chrome/.com.google.Chrome.gJY5h4', 194)
    I/LIBHOOK ( 8511): [8511] open('/dev/ashmem', 2)
    I/LIBHOOK ( 8511): [8511] open('/dev/ashmem', 2)
    I/LIBHOOK ( 8511): [8511] open('/data/data/com.android.chrome/shared_prefs/com.android.chrome_preferences.xml', 577)
    I/LIBHOOK ( 8511): [8511] open('/dev/ashmem', 2)
    I/LIBHOOK ( 8511): [8511] open('/dev/ashmem', 2)
    I/LIBHOOK ( 8511): [8511] open('/dev/ashmem', 2)
    I/LIBHOOK ( 8511): [8511] open('/data/data/com.android.chrome/files/android_ticl_service_state.bin', 0)
    I/LIBHOOK ( 8511): [8511] open('/data/data/com.android.chrome/files/ticl_storage.bin', 0)
    I/LIBHOOK ( 8511): [8511] open('/dev/ashmem', 2)
    I/LIBHOOK ( 8511): [8511] open('/dev/ashmem', 2)
    I/LIBHOOK ( 8511): [8511] open('/data/data/com.android.chrome/files/android_ticl_service_state.bin', 577)
    I/LIBHOOK ( 8511): [8511] open('/dev/ashmem', 2)
    I/LIBHOOK ( 8511): [8511] open('/dev/ashmem', 2)
    ...
    ...

## Note

Most of the ELF manipulation code inside the file hook.cpp of libhook was taken from the **Andrey Petrov**'s
blog post "[Android hacking: hooking system functions used by Dalvik](http://shadowwhowalks.blogspot.it/2013/01/android-hacking-hooking-system.html
)" and fixed by me ( the original source code didn't work due to page align, memory protection, etc ).

## License

Released under the BSD license.  
Copyright &copy; 2015, Simone Margaritelli <evilsocket@gmail.com>  
All rights reserved.
