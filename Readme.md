# GitPP - git in C++

Towards a pure modern (C++17ish) C++ re-implementation of git.

## Why!?

Are you crazy, why are you doing this?

My objectives are pretty simple :

* Have a git implementation that could be used with [include OS](http://http://www.includeos.org/).
* Get a better understand of all things git.
* Exercise my modern C++ muscles.
* Get to be offended by Linus Torvalds? (maybe.)

## What's here

* Parsing pack files.
* Parsing indexes files.
* Find objects by name on pack.
* Find objects by offset.
* Discover types for delta objects.
* Discover depth for delta objects.
* Read objects from packages.
* Read raw delta objects from the packages. (see parse deltas bellow)

## What need to be done

These are not in any particular order.

* Parse deltas.
    Deltas behave somewhat like and actual object, I believe that I need to refactor the delta object to have a method that returns the descriptor of the underlying deltified object.
* Stitch deltas together patch objects.
* Interpret different object types (blob, tree, commit and tag).
* Locate blob objects by commit/tree + path.
* Receive packages (be able to accept git push protocol).
    After this point gitpp can be used in a server/service environment.
* Git objects that are not on packs.
    After this point gitpp can be used on a client environment.
* Create objects.
* Pack objects.

## Samples

The library comes with a few samples code that you can try for yourself. They are small utilities that will execute some read-only operation on your git repo.

* ``pack_ls``
    Lists infromation about all the objects inside a package. Shows the same information that ``git verify-pack -v`` will with a little bit more verbosity. Also addresses in the packages are printed in hex to help locating them with hex-editors.

* ``pack_cat_obj``
    This will dump an object into the output. It could be used to extract blobs from the the package or to simply check them out. For the time being this will only work for non-delta objects.
