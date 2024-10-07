#include <algorithm>
#include <thread>
#include <execution>
#include <print>

#include "naive_parallelism.h"

void edible_mass_no_canibalism(const std::vector<int>& species_mass, const int growth_factor)
{
    //transform before reduction to avoid associativity problem
    std::vector<int> weight_per_species;
    std::vector<int> edible_mass(species_mass.size());
    std::ranges::transform(species_mass, std::back_inserter(weight_per_species), [&](const auto item){return item * growth_factor;});
    std::inclusive_scan(std::execution::par_unseq, weight_per_species.begin(), weight_per_species.end(), edible_mass.begin());

    std::print("\nFood amount avalilable for each species fixed:\n canibalism allowed (inclusive_sum): ");
    for(const auto& portion : edible_mass)
        std::print("{}, ", portion);

    //exclusive scan takes additional parameter (initial value) which will be assigned to very first element
    std::exclusive_scan(std::execution::par_unseq, weight_per_species.begin(), weight_per_species.end(), edible_mass.begin(), 0);

    std::print("\n canibalism not allowed (exclusive_sum): ");
    for(const auto& portion : edible_mass)
        std::print("{}, ", portion);
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
    
    std::print("\nFood amount avalilable for each species:\n partial_sum: ");
    for(const auto& portion : edible_mass)
        std::print("{}, ", portion);
    
    //parallelizable solution
    std::inclusive_scan(std::execution::par, species_mass.begin(), species_mass.end(), edible_mass.begin(),
	[growth_factor](const auto& smaller_pray, const auto& equal_pray){
		return equal_pray * growth_factor + smaller_pray;
    });

    std::print("\n inclusive_scan: ");
    for(const auto& portion : edible_mass)
        std::print("{}, ", portion);

    //naive parallelizable solution
    naive_inclusive_scan(species_mass, edible_mass,
	[growth_factor](const auto& smaller_pray, const auto& equal_pray){
		return equal_pray * growth_factor + smaller_pray;
    });

    std::print("\n naive inclusive_scan: ");
    for(const auto& portion : edible_mass)
        std::print("{}, ", portion);
    std::print("\n");

    /*
    //pipe it nicely together
    auto weighs = std::views::zip_transform(std::multiplies{}, species_population, species_weight);
    std::inclusive_scan(std::execution::par, weighs.begin(), weighs.end(), portions_scanned.begin(), [](const auto& prays_portions, const auto& prays_count){
        dummy_expensive_computation(prays_count);
        return prays_count + prays_portions;
        });
    
    std::cout << "food amount  avalilable for each species (transform piped insice inclusive_scan): ";
    for(const auto& portion : portions_scanned)
        std::cout << portion << ",";
    std::cout << "\n";
    */
}

void total_mass(const std::vector<int>& species_population, const std::vector<float>& species_weight)
{
    std::print("\nWhat is total weight of whole population: \n");
    std::print(" inner_product: {}g\n", std::inner_product(species_population.begin(), species_population.end(), species_weight.begin(), 0.f));
    std::print(" transform_reduce: {}g\n", std::transform_reduce(std::execution::par_unseq, species_population.begin(), species_population.end(), species_weight.begin(), 0.f));
    std::print(" naive parallelization: {}g\n", naive_transform_reduce(species_population.begin(), species_population.end(), species_weight.begin(), 0.f, std::multiplies{}, std::plus{}));
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
    
    std::print("\nPopulation multiplied by factor {}:\n", factor);
    std::print(" seq: {}\n reduce: {}\n naive reduce: {}\n transform_reduce: {}\n naive transform_reduce {}\n", next_gen_seq, next_gen_par, next_gen_par_naive, next_gen_par_fixed, next_gen_par_naive_fixed);
}

const auto list_of_species = std::ranges::iota_view{0, 1700};

void build_sky_empire()
{
    std::vector<std::jthread> creatures;

    //creates thread for everything
    std::for_each(list_of_species.begin(), list_of_species.end(), [&](const auto id){
        creatures.push_back(std::move(std::jthread([id]{
            std::print("my name is #{} and I'm flying!\n", id);
        })));
    });

}

void build_underwater_kingdom()
{
    //doesn't really make that much threads has clever management under the hood!
    std::for_each(std::execution::par, list_of_species.begin(), list_of_species.end(), [&](const auto id){
        std::print("my name is #{} and I'm swimming!\n", id);
    });
}

int main()
{
    //two different world made using two different approach - which one is better and why
    //build_sky_empire();
    //build_underwater_kingdom();

    //our world has nice natural chain - here is listed size of population of each of the species ordered from the smallest/weakest prays to the biggest predators
    std::vector<int> species_chain{68, 15, 4, 45, 18, 3, 2, 11};

    //how will the total population size change in the next generation if we will expect growth by given factor
    constexpr int factor = 2;
    next_generation_size(species_chain, factor);

    //make up list of weights for each kind. increasing row as probably larger are predators and smaller prays
    const std::vector<float> species_weight{0.0013f, 2.12f, 3.f, 43.3f, 512.2f, 6000.f, 7777777.7f, 150000000.23f};
    total_mass(species_chain, species_weight);

    //find out food options per species
    std::vector<int> species_mass(species_chain.size());
    std::vector<int> species_weight_simple;
    std::ranges::transform(std::ranges::iota_view{1, 9}, species_chain, std::back_inserter(species_weight_simple), std::multiplies{});
    std::partial_sum(species_weight_simple.begin(), species_weight_simple.end(), species_mass.begin());
    edible_mass_per_species(species_mass, factor);

    edible_mass_no_canibalism(species_mass, factor);
    
    return 0;
}