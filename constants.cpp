#pragma once
#include "createescrow.hpp"

namespace createescrow
{

using namespace eosio;
using std::string;
using std::vector;

/***
* Returns the symbol of the core token of the chain or the token used to pay for new account creation
* @return
*/
symbol create_escrow::getCoreSymbol()
{
    Token token(_self, _self.value);
    return token.begin()->S_SYS;
}

/***
     * Returns the contract name for new account action
     */
name create_escrow::getNewAccountContract()
{
    Token token(_self, _self.value);
    return token.begin()->newaccountcontract;
}

name create_escrow::getNewAccountAction()
{
    Token token(_self, _self.value);
    return token.begin()->newaccountaction;
}

/**
     * Returns the minimum bytes of RAM for new account creation
     */
uint64_t create_escrow::getMinimumRAM()
{
    Token token(_self, _self.value);
    return token.begin()->min_ram;
}

/***
* Returns the price of ram for given bytes
*/
asset create_escrow::getRamCost(uint64_t ram_bytes, uint64_t priceKey)
{
    asset ramcost;
    if (ram_bytes > 0)
    {
        RamInfo ramInfo(name("eosio"), name("eosio").value);
        auto ramData = ramInfo.find(S_RAM.raw());
        symbol coreSymbol = create_escrow::getCoreSymbol();
        check(ramData != ramInfo.end(), "Could not get RAM info [escrow.getRamCost]");

        uint64_t base = ramData->base.balance.amount;
        uint64_t quote = ramData->quote.balance.amount;
        ramcost = asset((((double)quote / base)) * ram_bytes, coreSymbol);
    }
    else
    { //if account is tier fixed
        ramcost = create_escrow::getTierRamPrice(priceKey);
    }
    return ramcost;
}

asset create_escrow::getTierRamPrice(uint64_t tierKey) {
    name newaccountcontract = create_escrow::getNewAccountContract();
    priceTable _prices(newaccountcontract, newaccountcontract.value);
    tierTable _tiers(newaccountcontract, newaccountcontract.value);
    auto priceitr = _prices.find(name("minimalaccnt").value);
    check(priceitr != _prices.end(), "No price found [escrow.getTierRamPrice]");
    auto tieritr = _tiers.find(tierKey);
    check(tieritr != _tiers.end(), "No tier found [escrow.getTierRamPrice]");
    asset price;
    price = priceitr->price;
    price.amount =  uint64_t(priceitr->price.amount * tieritr->ramfactor / 10000);
    return price;
}

asset create_escrow::getFixedCpu(uint64_t tierKey)
{
    name newaccountcontract = create_escrow::getNewAccountContract();
    tierTable _tiers(newaccountcontract, newaccountcontract.value);
    auto tieritr = _tiers.find(tierKey);
    check(tieritr != _tiers.end(), "No tier found [escrow.getFixedCpu]");
    return tieritr->cpuamount;
}

asset create_escrow::getFixedNet(uint64_t tierKey)
{
    name newaccountcontract = create_escrow::getNewAccountContract();
    tierTable _tiers(newaccountcontract, newaccountcontract.value);
    auto tieritr = _tiers.find(tierKey);
    check(tieritr != _tiers.end(), "No tier found [escrow.getFixedNet]");
    return tieritr->netamount;
}

auto create_escrow::getCpuLoanRecord(name account)
{
    rex_cpu_loan_table cpu_loans(name("eosio"), name("eosio").value);
    auto cpu_idx = cpu_loans.get_index<"byowner"_n>();
    auto loans = cpu_idx.find(_self.value);

    auto i = cpu_idx.lower_bound(_self.value);

    while (i != cpu_idx.end())
    {
        if (i->receiver == account)
        {
            return i;
        };
        i++;
    };

    check(false, ("No existing loan found for" + account.to_string() + "[escrow.getCpuLoanRecord]").c_str());
}

auto create_escrow::getNetLoanRecord(name account)
{
    rex_net_loan_table net_loans(name("eosio"), name("eosio").value);
    auto net_idx = net_loans.get_index<"byowner"_n>();
    auto loans = net_idx.find(_self.value);

    auto i = net_idx.lower_bound(_self.value);

    while (i != net_idx.end())
    {
        if (i->receiver == account)
        {
            return i;
        };
        i++;
    };

    check(false, ("No existing loan found for" + account.to_string() + "[escrow.getNetLoanRecord]").c_str());
}
} // namespace createescrow