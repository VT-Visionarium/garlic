#!/bin/bash

source ./mkEngines.bash

# See all_engines.x3d.txt for the features.  The features are tags like
# <IFfeatureName> where featureName is the arguments of the bash function
# called below.  Features are just blocks of XML code that are used or
# not.  It's like adding IF conditionals to manage the insertion of blocks
# to XML.

res="1920 1200"

AddFeatures head caveWalls caveStereo

