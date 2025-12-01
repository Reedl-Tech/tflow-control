echo "========  build.sh =========="
CURR_PATH=$(pwd)

echo "== 1 =="
cd /home/av/compulab-nxp-bsp/
pwd
export MACHINE=ucm-imx8m-plus
source setup-environment build-${MACHINE}

echo "== 2 =="
cd ${CURR_PATH}
pwd

echo "== 3 =="
bitbake -c $1 tflow-control

echo "========  EOF build.sh =========="