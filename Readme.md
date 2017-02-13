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

* parsing pack files.
* parsing indexes files.
* Find objects by name on pack.
* Find objects by offset.
* Discover types for delta objects.
* Discover depth for delta objects.
* read objects from packages.

## What need to be done

These are not in any particular order.

* interpret different object types (blob, tree, commit and tag).
* locate blob objects by commit/tree + path.
* receive packages (be able to accept git push protocol).
    After this point gitpp can be used in a server/service environment.
* git objects that are not on packs.
    After this point gitpp can be used on a client environment.
* create objects.
* pack objects.

