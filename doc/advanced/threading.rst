Multithreading
==============

There are some utilities for using AngelScript in the multithreading environment.

.. note::
   AngelScript does not support multithreading on all platforms.

   .. doxygenfunction:: asbind20::has_threads

Initializing and Cleaning the Multithreading Environment
--------------------------------------------------------

`According to the official document <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_adv_multithread.html#doc_adv_multithread_1>`_,
AngelScript needs some additional efforts to work in the multithreading environment.

The following functions are provided by the external header ``concurrent/threading.hpp``.

.. doxygenfunction:: asbind20::concurrent::auto_thread_cleanup
.. doxygenfunction:: asbind20::concurrent::prepare_multithread

Example code:

.. code-block:: c++

    #include <thread>
    #include <asbind20/asbind.hpp>
    #include <asbind20/concurrent/threading.hpp>

    int main()
    {
        if(!asbind20::has_threads())
        {
            std::cerr << "AS_NO_THREADS" << std::endl;
            return 1;
        }

        using namespace asbind20;
        concurrent::prepare_multithread();

        auto engine = make_script_engine();

        auto* m = engine->GetModule(
            "script_multithreading", asGM_ALWAYS_CREATE
        );
        m->AddScriptSection(
            "script_multithreading",
            "int fn(int arg) { return arg * 2; }"
        );
        m->Build();
        auto* f = m->GetFunctionByName("fn");

        std::condition_variable cv;
        std::mutex mx;
        int result = -1;

        auto helper = [&, f](int arg)
        {
            concurrent::auto_thread_cleanup();

            {
                using namespace std::chrono_literals;

                request_context ctx(engine);
                std::this_thread::sleep_for(3ms); // Emulates running a complex script
                auto r = script_invoke<int>(ctx, f, arg);
                std::unique_lock lock(mx);
                result = r.value();
            }

            cv.notify_all();
        };

        std::thread thr(helper, 10);
        thr.detach();
        {
            std::unique_lock lock(mx);
            std::cv_status st = cv.wait(lock);
        }
        assert(result == 20);

        return 0;
    }

Locks
-----

The AngelScript library provides two locks.

.. doxygenvariable:: asbind20::as_shared_lock
.. doxygenvariable:: asbind20::as_exclusive_lock

The weak reference flag (``asILockableSharedBool*``) is also lockable.

.. doxygenclass:: asbind20::lockable_shared_bool
   :members: lock, unlock
   :no-link:

Example code:

.. code-block:: c++

    {
        std::lock_guard guard(asbind20::as_exclusive_lock);
        // Do something...
    }

Atomic Reference Counting
-------------------------

.. doxygenclass:: asbind20::atomic_counter
   :members:
   :undoc-members:

For garbage collected types,
`please read the official document about thread safety and GC <https://www.angelcode.com/angelscript/sdk/docs/manual/doc_gc_object.html#doc_reg_gcref_4>`_.
