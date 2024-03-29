#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

# This script calls the SET action of createescrow and specifies the following
# Arguments: 1. CHAIN_SYMBOL:           core symbol of the chain on which createescrow is deployed
#            2. SYMBOL_PRECISION:       precision of core symbol
#            3. NEWACCOUNT_CONTRACT:    name of the contract to call for "newaccount" action. Generally eosio 
#            4. MINIMUM_RAM:            minimum bytes of RAM to put in a new account created on the chain 

#NOTE: This script assumes that you have the keys for createescrow account in your unlocked wallet

CHAIN_SYMBOL=${1:-EOS}
SYMBOL_PRECISION=${2:-4}
NEWACCOUNT_CONTRACT=${3:-eosio}
NEWACCOUNT_ACTION=${4:-newaccount}
MINIMUM_RAM=${5:-4096}

# specify the chain symbol and the contract name to call for new account action 
cleos push action createescrow init '["'$SYMBOL_PRECISION','$CHAIN_SYMBOL'","'$NEWACCOUNT_CONTRACT'","'$NEWACCOUNT_ACTION'","'$MINIMUM_RAM'"]' -p createescrow

