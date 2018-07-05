#include "market.hpp"

void market::offerpet(uuid pet_id, name newowner) {
    const auto& pet = pets.get(pet_id, "E404|Invalid pet");

    require_auth(pet.owner);
    eosio_assert(pet.owner != newowner, "new owner must be different than current owner");

    auto idx_existent_offer = offers.get_index<N(by_user_and_pet)>();
    auto user_pet_id = combine_ids(pet.owner, pet_id);
    auto itr_user_pet = idx_existent_offer.find(user_pet_id);

    if (itr_user_pet != idx_existent_offer.end()) {
        auto offer = *itr_user_pet;
        // eosio_assert(offer.type == 1, "you can't ask and bid at the same time. (should not happen anyway)");
        offers.modify(offer, pet.owner, [&](auto &r) {
            r.new_owner = newowner;
        });
    } else {
        uuid new_id = user_pet_id;
        offers.emplace(pet.owner, [&](auto& r){            
            st_offers offer{};
            offer.id = new_id;
            offer.user = pet.owner;
            offer.new_owner = newowner;
            offer.type = 1;
            offer.placed_at = now();
            r = offer;
        });
    }

    print("new owner can become ", newowner);
}

void market::removeoffer(name owner, uuid pet_id) {
    auto idx_existent_offer = offers.get_index<N(by_user_and_pet)>();
    const auto& offer = idx_existent_offer.get(combine_ids(owner, pet_id), "E404|Invalid offer");
    
    require_auth(owner);

    offers.erase(offer);
}

void market::claimpet(name oldowner, uuid pet_id) {
    auto idx_existent_offer = offers.template get_index<N(by_user_and_pet)>();
    const auto& offer = idx_existent_offer.get(combine_ids(oldowner, pet_id), "E404|Invalid offer");
    const auto& pet = pets.get(pet_id, "E404|Invalid pet");

    const name newowner = offer.new_owner;
    eosio_assert(newowner != (const name) {0}, "E404|Offer not personalized");
    require_auth(newowner);

    eosio_assert(oldowner == pet.owner, "E404|Pet already transferred");

    pets.modify(pet, newowner, [&](auto &r) {
        r.owner = newowner;
    });

    offers.erase(offer);
}