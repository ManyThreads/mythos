#! /bin/bash

# Copyright (c) 2014 Robert Kuban, BTU Cottbus-Senftenberg
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.


# settings

if [ -e COPYING ]
then
  licence='COPYING'
elif [ -e ../COPYING ]
then
  licence='../COPYING'
else
  echo "Can not find license file." 1>&2
  exit 1
fi
significant_lines=50

extensions="
h
cc
S
ld
rc
sh
asm
"

dirfilter="
./kernel/
"

# checks if a file includes the licence
# $1 filename
function check_file {
  local text=`head -n $significant_lines "$1" | tokenize`
  [[ "$text" =~ "$licence_text" ]]
}

# filter files by name
# $1 filename
function filter_name {
  for alright in $dirfilter
  do
    if [[ "$1" =~ "$alright" ]];
    then
      # true
      return 0
    fi
  done
  false;
}

# replace the first $1 lines of file $2 with the content of file $3
# $1 number of lines
# $2 file to modify
# $3 prefix file
function replace_prefix {
  local tmp_file=`mktemp`
  local start_line=$[$1+1]
  if tail -n +$start_line "$2" | cat "$3" - > "$tmp_file";
  then
    chmod --reference="$tmp_file" "$2"
    rm "$2"
    mv "$tmp_file" "$2"
  fi
}

function count_lines {
  cat "$1" | wc -l
}

function tokenize {
  tr -c '[:alnum:]' ' ' | tr -s '[:blank:]' ' '
}

# scan files
# output files with licence to stdout
# output files without licence to stderr
# $1 file extention
function scan {
  for filename in `find -regex .*\\\\.$1`;
  do
    if filter_name "$filename";
    then
      if check_file "$filename"
      then
        echo $filename
      else
        echo $filename 1>&2
      fi
    fi
  done
}
 
function scan_all {
  for ext in $extensions
  do
    scan "$ext"
  done
}

# global variables
licence_text=`cat $licence | tokenize`

# main
scan_all 2>&1 >/dev/null 
