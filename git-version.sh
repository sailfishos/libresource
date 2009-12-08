#!/bin/sh

if [ -d .git ]; then
    VERSION="`cat .git/HEAD`"
    case $VERSION in
        ref:*) ref="${VERSION#ref: }"; VERSION="`cat .git/$ref`";;
            *) ;;
    esac
fi

echo ${VERSION:-no git}
