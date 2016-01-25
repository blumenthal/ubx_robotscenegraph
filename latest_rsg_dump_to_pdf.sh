#!/bin/bash
#
# This script coverts the lates rsg dump file into a pdf by using thr dot tool
#
# Dependencies
# ------------
# * graphviz e.g. sudo apt-get isntall graphviz
# 
#
# Authors
# -------
#  * Sebastian Blumenthal (blumenthal@locomotec.com)

LATEST_FILE=`(ls rsg_dump* -t1 | head -n1)`
echo "Input file = ${LATEST_FILE}"

dot ${LATEST_FILE} -Tpdf -o ${LATEST_FILE}.pdf
