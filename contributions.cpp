#include <eosio/system.hpp>
#include <cstdlib>

#include "createescrow.hpp"

#include "lib/common.h"

#include "models/accounts.h"
#include "models/balances.h"
#include "models/registry.h"

#include "constants.cpp"

namespace createescrow
{
using namespace eosio;
using namespace std;

/*
* Returns the total balance contributed for a dapp
*/
asset create_escrow::balanceFor(string &memo)
{
    balance::Balances balances(_self, _self.value);
    uint64_t payerId = common::toUUID(memo);
    auto payer = balances.find(payerId);
    if (payer == balances.end())
        return asset(0'0000, create_escrow::getCoreSymbol());
    return payer->total_balance;
}

/*
* Checks whether the balance for an account is greater than the required balance
*/
bool create_escrow::hasBalance(string memo, const asset &quantity)
{
    return balanceFor(memo).amount > quantity.amount;
}

/*
     * Adds the amount contributed by the contributor for an app to the balances table
     * Called by the internal transfer function 
     */
void create_escrow::addBalance(const name &from, const asset &quantity, string &memo)
{

    vector<string> stats = common::split(memo, ",");
    auto dapp = stats[0];
    uint64_t id = common::toUUID(dapp);
    symbol core_symbol = create_escrow::getCoreSymbol();

    int ram = dapp == "free" ? 100 : stoi(stats[1]);
    int totalaccounts = stats.size() > 2 ? stoi(stats[2]) : -1;

    asset rex_balance = asset(0'0000, core_symbol);

    registry::Registry dapps(_self, _self.value);
    auto itr = dapps.find(common::toUUID(dapp));

    balance::Balances balances(_self, _self.value);
    auto iterator = balances.find(id);

    name newAccountContract = create_escrow::getNewAccountContract();

    // only owner or the whitelisted account are allowed to contribute for cpu and net
    // for globally available free funds, anybody can contribute
    if (create_escrow::checkIfOwner(from, dapp) || create_escrow::checkIfWhitelisted(from, dapp) || dapp == "free")
    {
        // cpu or net balance are passed in as 1000000 in memo for a value like 100.0000 SYS
        int64_t rex_quantity = stats.size() > 3 ? stoi(stats[3]) : 0'0000;

        rex_balance = asset(rex_quantity, core_symbol);

        // deposit rex funds for cpu and net
        if (rex_balance > asset(0'0000, core_symbol))
        {
            if (itr->use_rex == true)
            {
                action(
                    permission_level{_self, "active"_n},
                    newAccountContract,
                    name("deposit"),
                    make_tuple(_self, rex_balance))
                    .send();
            }
            else
            {
                check(false, ("Rex not enabled for " + dapp).c_str());
            }
        }

        asset balance = quantity - (rex_balance);

        if (iterator == balances.end())
            balances.emplace(_self, [&](auto &row) {
                row.memo = id;
                row.contributors.push_back({from, balance, rex_balance, asset(0'0000, core_symbol), asset(0'0000, core_symbol), asset(0'0000, core_symbol), ram, totalaccounts, 0});
                row.total_balance = balance;
                row.origin = dapp;
                row.timestamp = eosio::current_time_point().sec_since_epoch();
            });
        else
            balances.modify(iterator, same_payer, [&](auto &row) {
                auto pred = [from](const balance::contributors &item) {
                    return item.contributor == from;
                };
                std::vector<balance::contributors>::iterator itr = std::find_if(std::begin(row.contributors), std::end(row.contributors), pred);
                if (itr != std::end(row.contributors))
                {
                    itr->balance += balance;
                    itr->rex_balance += rex_balance;
                    itr->ram_contribution_percent = ram;
                    itr->totalaccounts = totalaccounts;
                }
                else
                {
                    row.contributors.push_back({from, balance, rex_balance, asset(0'0000, core_symbol), asset(0'0000, core_symbol), asset(0'0000, core_symbol), ram, totalaccounts, 0});
                    row.timestamp = eosio::current_time_point().sec_since_epoch();
                }
                row.total_balance += balance;
            });
    }
}

/*
     * Subtracts the amount used to create an account from the total amount contributed by a contributor for an app
     * Also checks if the memo account is same as one of the dapp contributors. If yes, then only increases the createdaccount field by 1
     * Called by the create action
     */
void create_escrow::subBalance(string memo, string &origin, const asset &quantity, bool memoIsDapp)
{
    uint64_t id = common::toUUID(origin);

    balance::Balances balances(_self, _self.value);
    auto iterator = balances.find(id);

    check(iterator != balances.end(), "No balance object");
    check(iterator->total_balance.amount >= quantity.amount, "overdrawn balance");

    balances.modify(iterator, same_payer, [&](auto &row) {
        auto pred = [memo](const balance::contributors &item) {
            return item.contributor == name(memo);
        };
        auto itr = std::find_if(std::begin(row.contributors), std::end(row.contributors), pred);
        if (itr != std::end(row.contributors))
        {
            row.total_balance -= quantity;
            itr->balance -= quantity;
            itr->total_spent += quantity;

            if (!memoIsDapp)
            {
                itr->createdaccounts += 1;
            }

            if (row.total_balance.amount <= 0 && itr->rex_balance.amount <= 0)
            {
                row.contributors.erase(itr);
            }
        }
        else
        {
            check(false, ("The account " + memo + "not found as one of the contributors for " + origin).c_str());
        }
    });
}

/* subtracts the balance used to stake/rex for cpu */
void create_escrow::subCpuOrNetBalance(string memo, string &origin, const asset &quantity, bool use_rex_balance)
{
    uint64_t id = common::toUUID(origin);

    balance::Balances balances(_self, _self.value);
    auto iterator = balances.find(id);

    check(iterator != balances.end(), "No balance object");

    balances.modify(iterator, same_payer, [&](auto &row) {
        auto pred = [memo](const balance::contributors &item) {
            return item.contributor == name(memo);
        };
        auto itr = std::find_if(std::begin(row.contributors), std::end(row.contributors), pred);
        if (itr != std::end(row.contributors))
        {
            check(itr->balance.amount >= quantity.amount, "overdrawn balance");
            itr->balance -= quantity;

            if (use_rex_balance)
            {
                itr->total_spent_rex += quantity;
            }
            else
            {
                itr->total_staked += quantity;
            }
        }
        else
        {
            check(false, ("The account " + memo + " not found as one of the contributors for " + origin).c_str());
        }
    });
}

/**********************************************/
/***                                        ***/
/***               Helpers                  ***/
/***                                        ***/
/**********************************************/

/***
     * Gets the balance of a contributor for a dapp
     * @return
     */
asset create_escrow::findContribution(string dapp, name contributor, string type)
{
    balance::Balances balances(_self, _self.value);
    uint64_t id = common::toUUID(dapp);
    auto iterator = balances.find(id);

    symbol coreSymbol = create_escrow::getCoreSymbol();

    auto msg = "No contribution found for " + dapp + " by " + contributor.to_string() + ".";

    // if no record found for the dapp in the balances table, return the balance for the contributor as 0
    if (iterator != balances.end())
    {
        auto pred = [contributor](const balance::contributors &item) {
            return item.contributor == contributor;
        };
        auto itr = std::find_if(std::begin(iterator->contributors), std::end(iterator->contributors), pred);
        if (itr != std::end(iterator->contributors))
        {
            if (type == "ram" || type == "net" || type == "cpu")
            {
                return itr->balance;
            }

            if (type == "rex")
            {
                return itr->rex_balance;
            }
        }
        else
        {
            print(msg.c_str());
            return asset(0'0000, coreSymbol);
        }
    }
    else
    {
        print(msg.c_str());
        return asset(0'0000, coreSymbol);
    }
}

/***
     * Gets the % RAM contribution of a contributor for a dapp
     * @return
     */
int create_escrow::findRamContribution(string dapp, name contributor)
{
    balance::Balances balances(_self, _self.value);
    uint64_t id = common::toUUID(dapp);
    auto iterator = balances.find(id);

    symbol coreSymbol = create_escrow::getCoreSymbol();

    auto msg = "No contribution found for " + dapp + " by " + contributor.to_string() + ". Checking the globally available free fund.";

    // if no record found for the dapp in the balances table, return the balance for the contributor as 0
    if (iterator != balances.end())
    {
        auto pred = [contributor](const balance::contributors &item) {
            return item.contributor == contributor;
        };
        auto itr = std::find_if(std::begin(iterator->contributors), std::end(iterator->contributors), pred);
        if (itr != std::end(iterator->contributors))
        {
            return itr->ram_contribution_percent;
        }
        else
        {
            print(msg.c_str());
            return 0;
        }
    }
    else
    {
        print(msg.c_str());
        return 0;
    }
}

/**
     * Randomly select the contributors for a dapp
     * It has following steps:
     * 1. Generate a random number with the new account name as the seed and the payer account name as the value
     * 2. Mod the individual digits in the number by the contributors vector size to get all the indices within the vector size
     * 3. Remove the duplicate indices 
     * 4. Chooses the contributors which have created accounts < total accounts until the max contribution upto 100% is reached
     */
vector<balance::chosen_contributors> create_escrow::getContributors(string origin, string memo, uint64_t seed, uint64_t to, asset ram)
{
    balance::Balances balances(_self, _self.value);
    auto iterator = balances.find(common::toUUID(origin));

    vector<balance::contributors> initial_contributors = iterator->contributors;
    vector<balance::contributors> final_contributors;
    vector<balance::chosen_contributors> chosen_contributors;
    vector<int> chosen_index;

    // generate a random number with new account name as the seed
    uint64_t number = common::generate_random(seed, to);

    int size = initial_contributors.size();

    // get the index from the contributors corresponding to individual digits of the random number generated in the above step
    while (size > 0 && number > 0)
    {
        int digit = number % 10;

        // modulus the digit of the random number by the initial_contributors to get the randomly generated indices within the size of the vector
        int index = digit % initial_contributors.size();

        // make sure not the same contributor is chosen twice
        if (std::find(chosen_index.begin(), chosen_index.end(), index) == chosen_index.end())
        {
            chosen_index.push_back(index);
            final_contributors.push_back(initial_contributors[index]);
        }
        number /= 10;
        size--;
    }

    int final_size = final_contributors.size();
    int i = 0;

    int max_ram_contribution = 100;
    int total_ram_contribution = 0;

    // choose the contributors to get the total contributions for RAM as close to 100% as possible
    while (total_ram_contribution < max_ram_contribution && i < final_size)
    {
        //check if the total account creation limit has been reached for a contributor
        if ((final_contributors[i].createdaccounts < final_contributors[i].totalaccounts || final_contributors[i].totalaccounts == -1) && final_contributors[i].contributor != name(memo))
        {

            int ram_contribution = findRamContribution(origin, final_contributors[i].contributor);
            total_ram_contribution += ram_contribution;

            if (total_ram_contribution > max_ram_contribution)
            {
                ram_contribution -= (total_ram_contribution - max_ram_contribution);
            }

            asset ram_amount = (ram_contribution * ram) / 100;

            chosen_contributors.push_back(
                {final_contributors[i].contributor,
                 ram_amount});
        }
        i++;
    }
    return chosen_contributors;
}

} // namespace createescrow