ncurest - an ncurses HTTP and REST client
=========================================

The goal is to create a simple interactive client program, that can be used to
test HTTP and in particular REST APIs.

![](ncurest-teaser.gif)

Dependencies
------------

ncurest depends on ncurses and has been developed using version 6.2-1.

Building and running ncurest
----------------------------

The build system is simply based on Makefiles. In the root of the repository
run

    make build

to build ncurest, which places the ncurest binary in the `bin` directory. Run
ncurest by executing

    make run

and clean the project by

    make clean

Testing ncurest
---------------

To run the test suite, simply execute

    make test

