#!/bin/bash

sourceDir=$1
fl=$2
ln=$3

cd $sourceDir

find . -name $fl | xargs sed "${ln}q;d"
