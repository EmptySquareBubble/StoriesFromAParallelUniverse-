#include <algorithm>
#include <thread>
#include <execution>
#include <print>
#include <ranges>

#include "naive_parallelism.h"

void edible_mass_avoid_sync(const std::vector<int>& species_population, const std::vector<int>& species_weight, int growth_factor)
{
    std::vector<int> portions_scanned(species_population.size());

    std::vector<int> grown_population(species_population.size());
    std::vector<int> grown_population_weight(species_population.size());
    std::transform(species_population.begin(), species_population.end(), grown_population.begin(), [&](const auto item){return item * growth_factor;});
    std::transform(grown_population.begin(), grown_population.end(), species_weight.begin(), grown_population_weight.begin(), std::multiplies{});
    std::exclusive_scan(std::execution::par_unseq, grown_population_weight.begin(), grown_population_weight.end(), portions_scanned.begin(), 0);

    std::print("\nFood amount  avalilable for each species piping\n synced between each step: ");
    for(const auto& portion : portions_scanned)
        std::print("{}g, ", portion);
    
    //pipe it nicely together - evaluation of zip_transform is on demand. no computation will be executed before item is really used
    auto weighs = std::views::zip_transform(std::multiplies{}, species_population, species_weight) | std::views::transform([&](const auto item){return item * growth_factor;});
    std::exclusive_scan(std::execution::par_unseq, weighs.begin(), weighs.end(), portions_scanned.begin(), 0);
    
    std::print("\n piped into exclusive_scan: ");
    for(const auto& portion : portions_scanned)
        std::print("{}g, ", portion);
    std::print("\n\n");
}

void edible_mass_no_canibalism(const std::vector<int>& species_mass, const int growth_factor)
{
    //transform before reduction to avoid associativity problem
    std::vector<int> weight_per_species;
    std::vector<int> edible_mass(species_mass.size());
    std::ranges::transform(species_mass, std::back_inserter(weight_per_species), [&](const auto item){return item * growth_factor;});
    
    std::inclusive_scan(std::execution::par_unseq, weight_per_species.begin(), weight_per_species.end(), edible_mass.begin());

    std::print("\nFood amount avalilable for each species fixed:\n canibalism allowed (inclusive_sum): ");
    for(const auto& portion : edible_mass)
        std::print("{}g, ", portion);

    //exclusive scan takes additional parameter (initial value) which will be assigned to very first element
    std::exclusive_scan(std::execution::par_unseq, weight_per_species.begin(), weight_per_species.end(), edible_mass.begin(), 0);

    std::print("\n canibalism not allowed (exclusive_sum): ");
    for(const auto& portion : edible_mass)
        std::print("{}g, ", portion);
    std::print("\n");
}

//count how much food is there available for each kind
//everyone can eat their kind and smaller
void edible_mass_per_species(const std::vector<int>& species_mass, const int growth_factor)
{
    std::vector<int> edible_mass(species_mass.size());

    int current_species_idx = 1;

    //serial solution
    std::partial_sum(species_mass.begin(), species_mass.end(), edible_mass.begin(), 
	[growth_factor](const auto& smaller_pray, const auto& equal_pray){
		return equal_pray * growth_factor + smaller_pray;
    });
    
    std::print("\nFood weight avalilable for each species next season:\n partial_sum: ");
    for(const auto& portion : edible_mass)
        std::print("{}g, ", portion);
    
    //parallelizable solution
    std::inclusive_scan(std::execution::par, species_mass.begin(), species_mass.end(), edible_mass.begin(),
	[growth_factor](const auto& smaller_pray, const auto& equal_pray){
		return equal_pray * growth_factor + smaller_pray;
    });

    std::print("\n inclusive_scan: ");
    for(const auto& portion : edible_mass)
        std::print("{}g, ", portion);

    //naive parallelizable solution
    naive_inclusive_scan(species_mass, edible_mass,
	[growth_factor](const auto& smaller_pray, const auto& equal_pray){
		return equal_pray * growth_factor + smaller_pray;
    });

    std::print("\n naive inclusive_scan: ");
    for(const auto& portion : edible_mass)
        std::print("{}g, ", portion);
    std::print("\n");
}

void total_mass(const std::vector<int>& species_population, const std::vector<float>& species_weight)
{
    std::vector<float> float_population;
    std::transform(species_population.begin(), species_population.end(), std::back_inserter(float_population), [](int x) { return static_cast<float>(x);});

    std::print("\nWhat is total weight of whole population: \n");
    std::print(" inner_product: {}g\n", std::inner_product(float_population.begin(), float_population.end(), species_weight.begin(), 0.f));
    std::print(" transform_reduce: {}g\n", std::transform_reduce(std::execution::par_unseq, float_population.begin(), float_population.end(), species_weight.begin(), 0.f));
    std::print(" naive parallelization: {}g\n", naive_transform_reduce(float_population.begin(), float_population.end(), species_weight.begin(), 0.f, std::multiplies{}, std::plus{}));
}

void next_generation_size(const std::vector<int>& species_population, const int factor)
{
    //simple serial version
    auto next_gen_seq = std::accumulate(species_population.begin(), species_population.end(), 0, [factor](const auto first, const auto second){
        return first + second * factor;});
    
    //not processed in order! wrong result
    auto next_gen_par = std::reduce(std::execution::par, species_population.begin(), species_population.end(), 0, [factor](const auto first, const auto second){
        return first + second * factor;});
    auto next_gen_par_naive = naive_reduce(species_population.begin(), species_population.end(), 0, [factor](const auto first, const auto second){
        return first + second * factor;});
    
    //fix it it with transform reduce
    auto next_gen_par_fixed = std::transform_reduce(std::execution::par, species_population.begin(), species_population.end(), 0, std::plus{}, [factor](const auto item){
        return item * factor;});
    auto next_gen_par_naive_fixed = naive_transform_reduce(species_population.begin(), species_population.end(), 0, [factor](const auto item){
        return item * factor;}, std::plus{});
    
    std::print("\nPopulation size multiplied by factor {}:\n", factor);
    std::print(" seq: {} residents\n reduce: {} residents\n naive reduce: {} residents\n transform_reduce: {} residents\n naive transform_reduce {} residents\n", next_gen_seq, next_gen_par, next_gen_par_naive, next_gen_par_fixed, next_gen_par_naive_fixed);
}

void build_sky_empire(const auto& list_of_species)
{
    std::vector<std::jthread> creatures;

    //creates thread for everything
    std::for_each(list_of_species.begin(), list_of_species.end(), [&](const auto id){
        creatures.push_back(std::move(std::jthread([id]{
            std::print("my name is #{} and I'm flying!\n", id);
        })));
    });

}

void build_underwater_kingdom(const auto& list_of_species)
{
    //doesn't really make that much threads has clever management under the hood!
    std::for_each(std::execution::par, list_of_species.begin(), list_of_species.end(), [&](const auto id){
        std::print("my name is #{} and I'm swimming!\n", id);
    });
}

void fish_and_shark()
{
    struct Hideout
    {
        int pos;
        //int slots;    //may be used only for std::execution::seq
        Hideout(int p, int c):pos(p), slots(c){}

        //all the rest is workaround to be able to use it with std::execution::par - not posible to use with unseq as there is synchronization included
        int slots_cnt()
        {
            std::lock_guard lock(slot_mutex);
            return slots;
        }
        bool take_slot()
        {
            std::lock_guard lock(slot_mutex);
            if(slots > 0)
            {
                --slots;
                return true;
            }
            return false;
        }
    private:
        int slots;
        std::mutex slot_mutex;        
    };

    struct Fish
    {
        int pos;
        int velocity;
    };

    ///comparison of different execution policies
    std::vector<Fish> fishes{{32, 10}, {54, 15}, {55, 5}, {123, 20}, {124, 18}, {124, 18}, {320, 2}, {323, 5}, {480, 16}};
    std::array<Hideout, 7> hideouts{Hideout{30, 1}, Hideout{54, 1}, Hideout{123, 1}, Hideout{145, 1}, Hideout{345, 1}, Hideout{423, 1}, Hideout{700, 1}};
    const auto time_interval = 1;
    std::atomic<bool> shark = true;
    
    std::print("Beware of shark!\n");
                
    //ok for par once shark is atomic or locked
    std::for_each(std::execution::par, fishes.begin(), fishes.end(), [&](auto& fish)
    {
        if(!shark)
        {
            fish.pos += time_interval * fish.velocity;
        }
        else
        {
            auto free_hideouts = std::ranges::filter_view(hideouts, [](auto& hideout){
                if (hideout.slots_cnt() > 0) 
                    return true; 
                return false;});
            
            auto nearest_hideouts = std::adjacent_find(free_hideouts.begin(), free_hideouts.end(), [&](const auto& first, const auto& second) { 
                if(first.pos <= fish.pos && second.pos >= fish.pos)
                    return true;
                return false;
            });

            if(nearest_hideouts == free_hideouts.end())
            {
                std::print(" fish at pos {} doesn't have any free hidout nearby\n", fish.pos);
                return;
            }

            const auto first = nearest_hideouts;
            const auto second = ++nearest_hideouts;
            
            auto& selected_hideout = fish.pos < std::midpoint(first->pos, second->pos) ? first : second;   
            const auto txt_begin = std::format(" fisht at pos {} runs to hideout at {}", fish.pos, selected_hideout->pos);
            
            //looong swim to hideout (to be able to see effect of spot stealing)
            const auto distance = std::abs(selected_hideout->pos - fish.pos);
            std::this_thread::sleep_for(std::chrono::milliseconds{distance * fish.velocity});

            if(selected_hideout->take_slot())
            {
                fish.pos = selected_hideout->pos;
                std::print("{} succesfully!\n", txt_begin);
            }
            else
                std::print("{} and arrives too late - hideout is full\n", txt_begin);     
        }
    });

    std::print("\n fishes final positions: \n  ");
    for(auto fish : fishes)
    {
        std::print("{}, ", fish.pos);
    }

    std::print("\n hideouts final state: \n  ");
    for(auto& hideout : hideouts)
    {
        std::print("[{}, {}]", hideout.pos, hideout.slots_cnt());
    }
}

int main()
{
    /*
    two different world made using two different approach
    if you create large number of species you can observe different behavior
    manual threads will drain your cpu while paralel alg version will stay sustainable thanks to thread management hidden behind
    commented out to make initial run of this example smooth - feel free to uncomment, but expect intensive load for your CPU
    */
    /*
    std::print("Create sky and underwater worlds\n");
    const auto list_of_species = std::ranges::iota_view{0, 1700};
    build_sky_empire(list_of_species);
    build_underwater_kingdom(list_of_species);
    */

    //our world has nice natural chain - here is listed size of population of each of the species ordered from the smallest/weakest prays to the biggest predators
    std::vector<int> species_chain{68, 15, 4, 45, 18, 3, 2, 11};
    //how will the total population size change in the next generation if we will expect growth by given factor
    constexpr int factor = 2;

    //get number of animals alive in next generation with given growth factor
    next_generation_size(species_chain, factor);

    //make up list of weights for each kind. increasing row as probably larger are predators and smaller prays
    const std::vector<float> species_weight{1e-6f, 2.1e-5f, 3.f, 43.3f, 5.1e3f, 6.5e4f, 7.7e6f, 1.5e8f};
    //get total mass of animals alive
    total_mass(species_chain, species_weight);
    
    //simplify weights to not deal with floating point values
    //smallest are of weight 1 next 2 etc..
    std::vector<int> species_mass;  //total mass per species
    std::ranges::transform(std::ranges::iota_view{1, 9}, species_chain, std::back_inserter(species_mass), std::multiplies{});
    //how much food will be available for each species in next generation with given growth factor - canibalism enabled
    edible_mass_per_species(species_mass, factor);

    //how much food will be available for each species in next generation with given growth factor - canibalism not enabled
    edible_mass_no_canibalism(species_mass, factor);
    
    //put multiple operations into one smooth chain without any sync between each step
    edible_mass_avoid_sync(species_chain, std::ranges::iota_view{1, 9} | std::ranges::to<std::vector>(), factor);

    //demonstrate different execution policies in one example
    //everyone is floating in given speed peacefully until shark attacks
    //then everyone looks for the closest hideout to stay safe 
    fish_and_shark();

    /*todo: 
    for_each a for_each_n je taky novy v 17
    transform_exclusive_scan + transform_inclusive_scan taky existuje (transform_cokoli_noveho)
    */

    return 0;
}