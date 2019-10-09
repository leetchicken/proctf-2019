build:

curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs -o rustup.sh

bash rustup.sh --default-toolchain none
source $HOME/.cargo/env

rustup default beta-2019-09-28
