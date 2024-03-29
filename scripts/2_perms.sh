#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

# This script adds the code level permission to the active permission of createescrow 
# Arguments: 1. PKEY: active key of createescrow

PKEY=${1:-EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV}

cleos set account permission createescrow active \
'{"threshold": 1,"keys": [{"key": "'$PKEY'","weight": 1}],"accounts": [{"permission":{"actor":"createescrow","permission":"eosio.code"},"weight":1}]}' owner
