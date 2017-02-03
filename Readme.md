# GitPP - git in C++

Towards a pure modern (C++17ish) C++ re-implementation of git.

## Why!?

Are you crazy, why are you doing this?

My objectives are pretty simple :

* Have a git implementation that could be used with [include OS](http://http://www.includeos.org/).
* Get a better understand of all things git.
* Exercise my modern C++ muscles.
* Get to be offended bt Linus Torvalds? (maybe.)

## What's here

* parsing pack files.
* parsing indexes files.
* Find objects by name on pack.
* Find objects by offset.
* Discover types for delta objects.
* Discover depth for delta objects.

## What need to be done

These are not in any particular order.

* read objects.
* index git objects that are not packaged.
* interpret different object types (blob, tree, commit and tag).
* create objects.
* pack objects.
* receive packages (be able to accept git push).
* locate read files by commit/tree + path.




