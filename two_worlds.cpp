#include <algorithm>
#include <ranges>
#include <thread>
#include <execution>
#include <iostream>
#include <chrono>
#include <numeric>
#include <syncstream>

constexpr int portion_size = 1;
constexpr int apetit_multiplier = 1;

void dummy_expensive_computation(int seed)
{
    double out = seed;
    for(int i = 0; i < seed * 1000; ++i)
        out += std::sqrt(i * std::rand());
}

//count how much food is there available for each kind
//everyone can eat their kind and smaller
void edible_mass_per_species(const std::vector<int>& species_population, const std::vector<int>& species_weight)
{
    int current_species_idx = 1;

    //serial solution
    std::vector<int> portions_sum(species_population.size());
    std::partial_sum(species_population.begin(), species_population.end(), portions_sum.begin(), [&current_species_idx, &species_weight](const auto& prays_portions, const auto& prays_count){
        const auto out = prays_count * species_weight[current_species_idx++] + prays_portions;
        return out;
        });
    
    std::cout << "food amount avalilable for each species (partial_sum): ";
    for(const auto& portion : portions_sum)
        std::cout << portion << ",";
    std::cout << "\n";

    //parallelizable solution
    //UB with other policy than std::execution::seq - concurrent acces to shared location current_species_idx - leaving here for better illustration 
    //(with seq policy there is no difference from partial sum so we can't observe all problems)
    current_species_idx = 1;
    std::vector<int> portions_scanned(species_population.size());
    std::inclusive_scan(std::execution::par, species_population.begin(), species_population.end(), portions_scanned.begin(), [&current_species_idx, &species_weight](const auto& prays_portions, const auto& prays_count){
        dummy_expensive_computation(prays_count);
        return prays_count * species_weight[current_species_idx++] + prays_portions;
        });
    
    std::cout << "food amount avalilable for each species (inclusive_scan): ";
    for(const auto& portion : portions_scanned)
        std::cout << portion << ",";
    std::cout << "\n";

    //transform earlier
    std::vector<int> weighed_population(species_population.size());
    std::transform(species_population.begin(), species_population.end(), species_weight.begin(), weighed_population.begin(), [](const auto cnt, const auto w){return cnt * w;});
    std::inclusive_scan(std::execution::par, weighed_population.begin(), weighed_population.end(), portions_scanned.begin(), [](const auto& prays_portions, const auto& prays_count){
        dummy_expensive_computation(prays_count);
        return prays_count + prays_portions;
        });
    
    std::cout << "food amount  avalilable for each species (transform and inclusive_scan): ";
    for(const auto& portion : portions_scanned)
        std::cout << portion << ",";
    std::cout << "\n";

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
}

int next_generation_size(const std::vector<int>& species_population, const int factor)
{
    //simple serial version
    auto next_gen_seq = std::accumulate(species_population.begin(), species_population.end(), 0, [factor](const auto first, const auto second){
        return first + second * factor;});
    //not processed in order! wrong result
    auto next_gen_par = std::reduce(std::execution::par, species_population.begin(), species_population.end(), 0, [factor](const auto first, const auto second){
        return first + second * factor;});
    //fix it it with transform reduce
    auto next_gen_par_fixed = std::transform_reduce(std::execution::par, species_population.begin(), species_population.end(), 0, std::plus{}, [factor](const auto item){
        return item * factor;});

    //little bit more sophisticated calculation of next gen
    //every spiecies wants to double its population, but also wants enough food for everyone 
    auto food_included = std::transform_reduce(std::execution::par, species_population.begin()+1, species_population.end(), species_population.begin(), species_population[0] * factor, std::plus{}, [factor](const auto& hunter, const auto& pray){
        const auto portions_available = pray / portion_size; 
        return std::clamp(portions_available, hunter, hunter * factor);});

    //check the results
    std::cout << "Population multiplied by factor " << factor << ":\n";
    std::cout << " seq: " << next_gen_seq << " reduce: " << next_gen_par << " transform_reduce: " << next_gen_par_fixed << std::endl;
    std::cout << "available food influences growth: " << food_included << std::endl;

    return next_gen_par_fixed;
}

const auto list_of_species = std::ranges::iota_view{0, 1700};

void build_sky_empire()
{
    std::vector<std::jthread> creatures;

    //creates thread for everything
    std::for_each(list_of_species.begin(), list_of_species.end(), [&](const auto id){
        creatures.push_back(std::move(std::jthread([id]{
            std::cout << "my name is #" << id << " and I'm flying!" << std::endl;
        })));
    });

}

void build_underwater_kingdom()
{
    //doesn't really make that much threads has clever management under the hood!
    std::for_each(std::execution::par, list_of_species.begin(), list_of_species.end(), [&](const auto id){
        std::cout << "my name is #" << id << " and I'm swimming!" << std::endl;
    });
}

int main()
{
    //two different world made using two different approach - which one is better and why
    //build_sky_empire();
    //build_underwater_kingdom();

    //our world has nice natural chain - here is listed size of population of each of the species ordered from the smallest/weakest prays to the biggest predators
    std::vector<int> species_chain{68, 15, 4, 45, 18, 3, 2, 11};

    //how will the total population size change in the next generation if we will expect growt by given factor
    constexpr int factor = 2;
    const auto creatures_cnt = next_generation_size(species_chain, factor);

    //make up list of weights for each kind. increasing row as probably larger are predators and smller prays
    //extra 1 at the end to make this example run even when i leave there nasty behaviour in first paralelizaion attempt
    const std::vector<int> species_weight{1, 2, 3, 4, 5, 6, 7, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    //find out food options per species
    edible_mass_per_species(species_chain, species_weight);

    return 0;
}