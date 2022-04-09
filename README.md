# Logger
An enthralling logger written in C++.
The logger class is immovable and non-copyable. Where you place it is where it lives.
As it should be with logger classes. Writes are atomic, but order is not guaranteed.
For the moment, messages are blocking.

```
Logger myLogger( "My Logger", "/tmp/my_application/class_my_logger.log" );

...

myLogger.debug( "The number of threads being created is %lu", numberThreads );

...

myLogger.critical( "The program is in an invalid state, calling exit()" );
exit( EXIT_FAILURE );
```

At this point you should just go read the API_Logger.uml file to see the rest of the API.
It's pretty straight forward.
