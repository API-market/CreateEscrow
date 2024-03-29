
# Create Escrow - A subsidized account creation contract
It is not trivial to fund account creation on EOS. This contract simplifies the account creation for dapp developers.
So they can focus on developing dapps and not worry about the complexity of account creation.

## Scenario 1
A wallet app wants to onboard new user accounts for a dapp at 20% cost subsidy for first 1000 accounts. It can do this by specifying the above requirements to [createescrow](#createescrow).
createescrow also makes sure that the funds are accounted for and only used for the dapp specified by the wallet app. 

## Scenario 2
Developer `A` wants to contribute 50% of RAM cost for new user accounts for a dapp. Developer `B` also wants to contribute 50%.
[createescrow](#createescrow) enables them to do so by getting 50% cost each from `A` and `B` when it creates the new user accounts.

## Scenario 3
`Block.One` donates some EOS to the community for free account creation. There is no place to do that right now. But with [createescrow](#createescrow) they can do this and also get an account for where those funds get used.

### createescrow
`createescrow` is an EOSIO account creator contract that allows dapps to assume portions of the costs for account creation.

As a dapp owner, you can define your dapp with `createescrow` contract specifying the owner/whitelisted account for the app. You can also specify the amount of RAM, NET and CPU you want for the new user accounts. 
As somebody who wants to contribute towards account creation whether it's the dapp owner or any other account, you can send the `createescrow` contract funds with the dapp name and the percentage of RAM you want to subsidize in the memo. These funds will be used to pay for the user account.

Any contributor can reclaim their remaining balance at any point of time.

👉🏻 **Please see the [Quick Start](#quick-start) section for getting started and more detailed explanation of its working here**

## Feature
Taking the lead from the scatter team, we have added some more features:
- Allow only the accounts registered with the dapp to avail the funds contributed for the dapp.
- Easy airdrops of dapp specific tokens with new account creation.
- No risk of loosing your contribution. Reclaim remaining balance at any point of time.
- Easy access to globally available free fund.
- Selection of multiple contributors to maximise the % of RAM cost subsidized for a new account.
- Enables user to specify the total number of accounts to subsidize for a dapp.

### Define your dapp with createescrow

If you are a creator of a dapp, you can define a unique identifier for the dapp and register the `owner` account name for the dapp. 
You can also specify the RAM in bytes, the amount of EOS for NET and CPU (in that order) to give to the new user accounts. 
Also, if you wish to airdrop dapp tokens to the newly created account specify the airdrop contract, total airdrop supply
and the amount to airdrop to every new user account created. 
```
cleos push action createescrow define '[YOUR_ACCOUNT,dapp_name,ram_bytes,net,cpu,[airdrop_contract,airdrop_tokens,airdrop_limit],use_rex,[rex_net_loan_payment,rex_net_loan_fund,rex_cpu_loan_payment,rex_cpu_loan_fund]]'

cleos push action createescrow define '["mydappowner1","mydapp.org","1.0000 EOS","1.0000 EOS","1.0000 EOS",["mydapptoken1","1000000.0000 DP","10.0000 DP"],false,["0.0000 EOS","0.0000 EOS","0.0000 EOS","0.0000 EOS"]]'
```
### Fund createescrow

You can contribute to a dapp by specifying its name, the percentage of RAM cost you want to contribute, the total number of accounts you want to create, funds for cpu and funds for net separated by comma in the memo field for transfer action. Please note the cpu and net funds are denoted as 10000 for 1.0000 in the memo.
```
cleos transfer YOUR_ACCOUNT createescrow "token_balance" "dapp_name,ram_percentage,_total_accounts,cpu_fund,net_fund"

cleos transfer YOUR_ACCOUNT createescrow "10.0000 EOS" "mydapp.org,50,100,10000,10000"

cleos transfer YOUR_ACCOUNT createescrow "10.0000 EOS" "mydapp.org,100,100,10000,10000"
```

### Create new accounts
To create accounts using funds contributed to a dapp
```
cleos push action createescrow create '["dapp_owner","account_name","owner_key","active_key","dapp_name"]'`

cleos push action createescrow create '["mydappowner1","mydappuser11","EOS4xJvy2tYU21reKbbq4RPLxgzxNmrLtidVWpio5Ggwisfkgzg2L","EOS4xJvy2tYU21reKbbq4RPLxgzxNmrLtidVWpio5Ggwisfkgzg2L","mydapp.org"]'
```

To use globally available free funds
```
cleos push action createescrow create '["memo", "account_name","owner_key","active_key","free"]'
```
```
cleos push action createescrow create '["using free funds","mydappuser12","EOS4xJvy2tYU21reKbbq4RPLxgzxNmrLtidVWpio5Ggwisfkgzg2L","EOS4xJvy2tYU21reKbbq4RPLxgzxNmrLtidVWpio5Ggwisfkgzg2L","free"]'
```

### Whitelist other accounts

You can whitelist other accounts to create accounts on behalf of your dapp.
```
cleos push action createescrow whitelist '[YOUR_ACCOUNT,account_name,dapp_name]'
cleos push action createescrow whitelist '["mydappowner1",account_name,"mydapp.org"]'
```

### Globally Crowd-Free Accounts

Anyone can send EOS to this contract with the memo __free__ to help fund completely free accounts for new EOS users.
```
cleos transfer YOUR_ACCOUNT createescrow "10.0000 EOS" "free"
```

### Reclaim balance
- As a contributor, you can reclaim your balance at any time.
- You can specify the token symbol of the balance you want to reclaim:
    - Core symbol token to reclaim core symbol balance
    - Dapp token symbol to reclaim the remaining airdrop balance. Only to be reclaimed by the dapp owner
```
cleos push action createescrow reclaim '["YOUR_ACCOUNT","dapp_name","token_symbol"]'

cleos push action createescrow reclaim '["mydappowner1","mydapp.org","EOS"]'

cleos push action createescrow reclaim '["mydappowner1","mydapp.org","DP"]'
```

### Everyone can help!

Because of the way this contract works, anyone can help fund account creation for your app by simply sending amounts
to the contract with the dapp origin inside of the memo.
```
cleos transfer YOUR_ACCOUNT createescrow "10.0000 EOS" "mydapp.org,50"
```

### Quick Start:
- Attach the code level permission to active permission of createescrow
```
cleos set account permission createescrow active '{"threshold": 1,"keys": [{"key": "ACTIVE_KEY","weight": 1}],"accounts": [{"permission":{"actor":"createescrow","permission":"eosio.code"},"weight":1}]}' owner`
```
- Specify the token symbol for the chain and also the contract to call for newaccount action
```
cleos push action createescrow set '["CORE_SYMBOL","NEWACCOUNT_CONTRACT"]'

cleos push action createescrow set '["EOS","eosio"]' -p createescrow
```

Also checkout the detailed steps in the scripts directory and end to end functionalities in test directory.

## Deploy createescrow
Run scipts/deploy.sh to deploy createescrow on EOS jungle, kylin, mainnet and ORE chain networks.