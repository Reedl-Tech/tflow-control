echo "========  build.sh =========="
CURR_PATH=$(pwd)

echo "== 1 =="
cd /home/av/imx/
pwd
MACHINE=ucm-imx8m-plus
source compulab-setup-env -b build-${MACHINE}

echo "== 2 =="
cd ${CURR_PATH}
pwd

echo "== 3 =="
bitbake -c $1 tflow-control

#echo "== 4 =="
#cat /home/av/imx/build-ucm-imx8m-plus/tmp/work/cortexa53-crypto-poky-linux/tflow-process/1.0-r0/temp/log.do_compile >&2

echo "========  EOF build.sh =========="