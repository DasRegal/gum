#!/bin/sh

RUNIT_VERSION=1.0V

INC_FILE_NAME=includes
INC_FILES=$1
EXE_FILE=runit_exe
RUNIT_PATH=$(pwd)
PRJ_PATH="$RUNIT_PATH/.."
SPEC_DIR="$PRJ_PATH/spec"

echo "RUnit $RUNIT_VERSION"

if [ ! -d $SPEC_DIR ]
then
    echo
    echo "[RUnit] Error: \`$SPEC_DIR\` directory does not exists."
    exit 1
fi

if [ -f $INC_FILE_NAME ]
then
    rm $INC_FILE_NAME
fi
touch $INC_FILE_NAME

if [ -z "$INC_FILES" ]
then
    INC_FILES=`ls $SPEC_DIR/*`
fi

SRC=""

for eachfile in $INC_FILES
do
    echo "#include \"$eachfile\"" >> $INC_FILE_NAME
    INC_SRC=$(cat $eachfile | grep -Ei 'source\(\".*"\)' | grep -oEi '\".*\"' | grep -oEi '[^"]*' | tr '\n' ' ')
    if [ ! -z "$INC_SRC" ]
    then
        SRC="$SRC $PRJ_PATH/$INC_SRC"
    fi
done

gcc $RUNIT_PATH/runit.c $SRC -o $RUNIT_PATH/$EXE_FILE -DTEST_MDB
if [ -f $INC_FILE_NAME ]
then
    $RUNIT_PATH/$EXE_FILE
else
    echo
    echo "[RUnit] Error: compile error."
    exit 1
fi
# rm $INC_FILE_NAME
rm $EXE_FILE