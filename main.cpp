#include <iostream>
#include <chrono>
#include <fstream>
#include <string_view>
#include <vector>
#include <initializer_list>
#include <array>

using uint = unsigned int;

namespace pcg
{
    struct pcg32_random_t
    {
        uint64_t state;
        uint64_t inc;
    } random;

    void init(uint64_t state, uint64_t inc)
    {
        random.state = state;
        random.inc = inc;
    }

    uint32_t pcg32_random_r(pcg32_random_t &rng)
    {
        auto oldState = rng.state;

        // Advance internal state
        rng.state = oldState * 6364136223846793005ULL + (rng.inc | 1);

        // Calculate output function (XSH RR), uses old state for max ILP
        uint32_t xorShifted = ((oldState >> 18u) ^ oldState) >> 27u;
        uint32_t rot = oldState >> 59u;

        return (xorShifted >> rot) | (xorShifted << ((-rot) & 31));
    }

    uint get_random() { return pcg32_random_r(random); }
}

uint g_depth;

enum class Ingredient
{
    Tomato,
    Mushroom,
    Used
};

struct Pizza
{
    const uint rows;
    const uint cols;
    const uint maxCells;
    const uint minIngredients;
    std::vector<Ingredient> ingredients;

    Ingredient& get(const uint row, const uint col) { return ingredients[row * cols + col]; }

    const Ingredient& get(const uint row, const uint col) const { return ingredients[row * cols + col]; }

    void mask_piece(const uint row, const uint col) { ingredients[row * cols + col]; }

    template<class Iter1, class Iter2>
    Pizza(const Iter1 iterBegin, const Iter2 iterEnd, const uint rows, const uint cols, const uint ingredientsMin, const uint maxCells) :
            ingredients(iterBegin, iterEnd),
            rows(rows),
            cols(cols),
            minIngredients(ingredientsMin),
            maxCells(maxCells)
    {}

    static Pizza from_file(const std::string_view& filename)
    {
        std::fstream file;
        file.open(filename.data());

        uint rows;

        file >> rows;

        uint cols;

        file >> cols;

        uint minIngredient;

        file >> minIngredient;

        uint maxCells;

        file >> maxCells;

        std::vector<Ingredient> ingredients(rows * cols);

        uint iter = 0;

        while (!file.eof())
        {
            char c;
            file >> c;

            switch (c)
            {
                case 'T':
                    ingredients[iter] = Ingredient::Tomato;
                    break;

                case 'M':
                    ingredients[iter] = Ingredient::Mushroom;
                    break;
            }

            iter++;
        }

        return Pizza(ingredients.begin(), ingredients.end(), rows, cols, minIngredient, maxCells);
    }
};

struct Slice
{
    const uint rowBegin;
    const uint rowEnd;
    const uint columnBegin;
    const uint columnEnd;
    Pizza* pizza;

    Slice(Pizza* pizza, const uint rowBegin, const uint rowEnd, const uint columnBegin, const uint columnEnd) :
            pizza(pizza),
            rowBegin(rowBegin),
            rowEnd(rowEnd),
            columnBegin(columnBegin),
            columnEnd(columnEnd)
    { }

    static Slice random_slice(Pizza& pizza)
    {
        //uint r1 = (uint)pcg::get_random() % pizza.rows;
        //uint c1 = (uint)pcg::get_random() % pizza.cols;
        //uint r2 = (uint)pcg::get_random() % pizza.rows;
        //uint c2 = (uint)pcg::get_random() % pizza.cols;

        //slice.rowBegin = r1 <= r2 ? r1:r2;
        //slice.rowEnd = r1 > r2 ? r1:r2;
        //slice.columnBegin = c1 <= c2 ? c1:c2;
        //slice.columnEnd = c1 > c2 ? c1:c2;

        const auto rowBegin = (uint) pcg::get_random() % pizza.rows;
        const auto columnBegin = (uint) pcg::get_random() % pizza.cols;

        uint rowEnd = 0;
        uint columnEnd = 0;

        do rowEnd = rowBegin + (uint) pcg::get_random() % (pizza.maxCells + 1);
        while (rowEnd >= pizza.rows);

        do columnEnd = columnBegin + (uint) pcg::get_random() % (pizza.maxCells + 1);
        while (columnEnd >= pizza.cols);

        return Slice(&pizza, rowBegin, rowEnd, columnBegin, columnEnd);
    }

    bool check(Pizza& pizza) const
    {
        uint mushrooms = 0;
        uint tomatoes = 0;

        for (uint i = rowBegin; i <= rowEnd; ++i)
        {
            for (uint j = columnBegin; j <= columnEnd; ++j)
            {
                switch (pizza.get(i, j))
                {
                    case Ingredient::Tomato: tomatoes++; break;
                    case Ingredient::Mushroom: mushrooms++; break;
                    case Ingredient::Used: return false;
                }
            }
        }

        return tomatoes >= pizza.minIngredients && mushrooms >= pizza.minIngredients && (tomatoes + mushrooms) <= pizza.maxCells;
    }

    void mark_as_used()
    {
        for (uint i = rowBegin; i <= rowEnd; i++)
        {
            for (uint j = columnBegin; j <= columnEnd; j++)
            {
                pizza->get(i, j) = Ingredient::Used;
            }
        }
    }
};

uint score_slices(const std::vector<Slice>& slices)
{
    uint score = 0;

    for (const Slice& slice : slices)
        score += (slice.rowEnd - slice.rowBegin + 1) * (slice.columnEnd - slice.columnBegin + 1);

    return score;
}

std::vector<Slice> generate_array_of_slices(Pizza pizza)
{
    std::vector<Slice> slices;
    uint depth = g_depth;

    while (depth != 0)
    {
        auto temp = Slice::random_slice(pizza);

        if (temp.check(pizza))
        {
            slices.push_back(temp);

            temp.mark_as_used();
            depth = g_depth;
        }

        depth--;
    }

    return slices;
}

int main(int argc, char **argv)
{
    //pcg::init(atoi(argv[2]), 54u);
    pcg::init(1080833423459783, 54u);

    //g_depth = atoi(argv[3]);
    g_depth = 50000000;

    Pizza pizza = Pizza::from_file("medium.in");

    size_t max_score = 0;
    uint score;

    uint64_t n = 0;

    while (true)
    {
        auto slices = generate_array_of_slices(pizza);

        score = score_slices(slices);

        if (max_score < score)
        {
            max_score = score;

            std::fstream file;
            std::remove(argv[4]);

            file << slices.size() << '\n';
            file.open(argv[4], std::fstream::out);

            std::cout << score << '\n';

            for (const auto &slice : slices)
            {
                file << slice.rowBegin << ' ';
                file << slice.columnBegin << ' ';
                file << slice.rowEnd << ' ';
                file << slice.columnEnd << ' ' << '\n';
            }

            file.close();
        }

        n++;
    }

    return 0;
}
