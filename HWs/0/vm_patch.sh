#!/bin/bash

<<END_COMMENT
This is a script to change the spring2020 VM to the spring2022 one
automatically. It removes the old handouts directory and initializes
the personal repository in the ~/code/personal directory so that the
students fetch updates from "handouts" remote and push their changes
to "origin" remote.

END_COMMENT

HW_HANDOUTS_REPO="git@tarasht.ce.sharif.edu:ce424-002-students/ce424-002-handouts.git"

# Part 1: update handouts and delete previous year handouts
cd /home/vagrant/code/
rm -rf handouts
git clone $HW_HANDOUTS_REPO
mv ce424-002-handouts handouts

# Part 2: initialize handouts repository inside personal folder
cd /home/vagrant/code/personal
rm -rf .git
git init
git remote add handouts $HW_HANDOUTS_REPO

# Part 3: set origin repository
read -p "Enter your stundent ID: " sid
STUDENT_REPO="git@tarasht.ce.sharif.edu:ce424-002-students/ce424-002-$sid.git"
git remote add origin $STUDENT_REPO

