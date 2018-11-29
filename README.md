yalp
====

[![Build Status](https://travis-ci.org/tyfkda/yalp.svg?branch=master)](https://travis-ci.org/tyfkda/yalp)

Yet Another List Processor

Under construction.

## Features

* Lisp-1 (same name space for function and variable)
* Case sensitive symbol
* Continuation
* Macro / Read macro
* Self hosting compiler
* Garbage collection (mark & sweep)

## No
* Bignum / Rational number / Complex number
* Keyword

## How to build
### Mac, Linux
* `make`
* `make test`

### Windows (Visual Studio 2012)
* Open build/yalp.vcproj
* [Build]


## Design

* Keep simple
* Embeddable
* Practical
* Can be used for shell scripting language

## Release
### v007
* Optimize quasiquote expander
* Allocate stack work space at function top
* Avoid stack copy for continuation which executes nonlocal exit
* Generate loop code for global function self recursion

### v006
* Generate loop code for tail recursion

### v005
* Add function binder
* Chaned to Common Lisp like syntax
* Support inline function

### v004
* Add multiple values
* Add stream
* Add read macro (set-macro-character)
