#! /bin/sh

cd /pub/albatross

CROSS=
BUILDLIST=
BUILDALL=

while [ "$#" -ne 0 ]; do

     case $1 in

         --loud)    shift
                    LOUD_FLAG="Q="
                    ;;

         --clean)   shift
                    MAKE_CLEAN=Y
                    MAKE_CLEAN_ONLY=Y
                    ;;

         --install) shift
                    if [ "$1" = "" -o "${1:0:1}" = "-" ]; then
                        echo "Error: --install option requires next parameter to be the installation directory" 
                        exec false
                    else
                        INSTALLDIR=$1
                        shift
                    fi
                    INSTALL=Y
                    ;;

         --rebuild) shift
                    MAKE_CLEAN=Y
                    MAKE_CLEAN_ONLY=
                    ;;

         --strip)   shift
                    STRIP_EXE=Y
                    ;;

         --arm)     shift
                    PATH=$PATH:/opt/arm/bin
                    CROSS=arm-linux-
                    KERNELDIR=${KERNELDIR-/pub/sysapps/src/linux-2.6.19}
                    ;;

         --i386)    shift
                    KERNELDIR=${KERNELDIR-/pub/fc2/linux-2.6.11.10}
                    ;;

         all)       shift
                    BUILDALL=1
                    ;;

         --*)       echo "Unrecognized switch: $1" 
                    shift
                    ;;

         *)         BUILDLIST="$BUILDLIST $1"
                    shift
                    ;;
     esac

done

if [ -z "$KERNELDIR" ]; then
    echo "Error: KERNELDIR= is missing."
    exec false
fi

[ -n "$INSTALLDIR" ] && mkdir -p ${INSTALLDIR}

if [ "${TERM}" == "dumb" ]; then
    SMSO='['
    RMSO=']'
else
    SMSO=`tput smso`
    RMSO=`tput rmso`
fi

function install()
{
    echo "  PWD    " `pwd`
    echo "  COPY   " $1 "->" $2
    cp -a $1 $2
    if [ $? -ne 0 ]; then
        echo "Copy failed!"
        exec false
    fi
}

function build()
{
    name=$1
    shift

    CURWD=`pwd`
    cd $name 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "-------------------------------------------------- $SMSO $name $RMSO"
        echo "Error: Directory \"$name\" not found. Skipping to next."
        return
    fi

    echo "-------------------------------------------------- $SMSO $name $RMSO"
    echo "  PWD    " `pwd`

    [ -z "${MAKE_CLEAN}" ] || make clean ${LOUD_FLAG} 2>/dev/null

    if [ -z "${MAKE_CLEAN_ONLY}" ]; then

        if [ -n "${INSTALL}" ]; then
            make install CROSS=${CROSS} INSTALLDIR=${INSTALLDIR} STRIP_EXE=${STRIP_EXE} ${LOUD_FLAG} $@
        else
            make CROSS=${CROSS} STRIP_EXE=${STRIP_EXE} ${LOUD_FLAG} $@
        fi
        if [ $? -ne 0 ]; then
            echo "Build failed!"
            exec false
        fi
    fi

    cd $CURWD
}

# If build list contains backslash characters (which is case when called from PN2
# over PuTTY's plink, i.e. ssh, and the argument is full path of the directory in
# windows format), then we get 'basename' directory
#
if echo ${BUILDLIST} | grep '\\'  >/dev/null; then
    BUILDLIST=`echo ${BUILDLIST} | sed 's/\\\\/\\//g'`
    BUILDLIST=`basename ${BUILDLIST}`
fi

if [ -n "${BUILDLIST}" ] && [ -z "${BUILDALL}" ]; then

    for project in $BUILDLIST; do
        if [ "$project" == "hpi" ]; then
            build hpi KERNELDIR=${KERNELDIR}
        else
            build $project
        fi
    done

else

    build hpi KERNELDIR=${KERNELDIR}

    build c54setp
    build c54load
    build idod
    build idodMaster
    build idodTrace
    build idodVR
    build webStat
    build ipSetup
    build rtpPing
    build webs
    build webs/cgi-src INSTALLDIR=
    build webs/java-src INSTALLDIR=

    # Manual installation

    if [ -n "${INSTALL}" ]; then

        echo "-------------------------------------------------- $SMSO DSP $RMSO"
        install dsp/GE2P/Debug/ge2p.out ${INSTALLDIR}/ge2p.x54

        echo "-------------------------------------------------- $SMSO WEB ROOT $RMSO"
        install webs/web $INSTALLDIR/../web

    fi

fi # BUILDLIST / BUILDALL

echo "-------------------------------------------------- $SMSO Done. $RMSO"

