#!/bin/bash
#
# This script coverts the lates rsg dump file into a pdf by using thr dot tool.
#
# Dependencies
# ------------
# * graphviz e.g. sudo apt-get isntall graphviz
#
# Usage
# -----
# Call this script simply without any parameters:
# ./latest_rsg_dump_to_pdf.sh
#
# OR call it and show it immediatly in a pdf viewer (here evince):
#
# evince `(./latest_rsg_dump_to_pdf.sh)`
# 
#
# Authors
# -------
#  * Sebastian Blumenthal (blumenthal@locomotec.com)

LATEST_FILE=`(ls rsg_dump* -t1 | head -n1)`

dot ${LATEST_FILE} -Tpdf -o ${LATEST_FILE}.pdf
echo "${LATEST_FILE}.pdf"
