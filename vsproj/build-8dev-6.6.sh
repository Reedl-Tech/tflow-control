echo "========  build.sh =========="
CURR_PATH=$(pwd)
locale
echo "== 1 =="
export LANG=en_US.UTF-8
locale

cd /home/av/8dev-Scarthgap/compulab-nxp-bsp/
pwd
export MACHINE=ucm-imx8m-plus-bubo
source reedl-init-build-env build-${MACHINE}

echo "== 2 =="
cd ${CURR_PATH}
pwd

echo "== 3 =="
bitbake -c $1 tflow-control

echo "========  EOF build.sh =========="