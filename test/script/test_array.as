class my_pair
{
    int x;
    int y;

    my_pair()
    {
        x = 0;
        y = 0;
    }

    my_pair(int x, int y)
    {
        this.x = x;
        this.y = y;
    }

    bool opEquals(const my_pair&in other) const
    {
        return this.x == other.x && this.y == other.y;
    }
};

void my_pair_array()
{
    array<my_pair> pairs = {my_pair(1, 1), my_pair(2, 2)};

    print("pairs[0].x =" + to_string(pairs[0].x));
    print("pairs[0].y =" + to_string(pairs[0].y));
    assert(pairs[0] == my_pair(1, 1));
    assert(pairs[1] == my_pair(2, 2));

    print("Adding new value my_pair(3, 3)");
    pairs.push_back(my_pair(3, 3));
    assert(pairs.size == 3);
    assert(pairs.back == my_pair(3, 3));
}

void my_pair_ref_array()
{
    my_pair p1 = my_pair();
    my_pair p2 = my_pair(1, 2);

    array<my_pair@> pairs = {p1, p2};
    print("pairs created");
    assert(pairs.size == 2);
    for(int i = 0; i < pairs.size; ++i)
    {
        my_pair@ p = pairs[i];
        print("pairs[" + to_string(i) + "] = " + to_string(p.x) + ", " + to_string(p.y));
    }

    assert(pairs.front == my_pair(0, 0));
    print("new pair value assigned");

    pairs[0].y = 1;
    print("pairs[0].y = 1");
    assert(pairs == {my_pair(0, 1), my_pair(1, 2)});
}

void print_string_array(array<string>@ strs)
{
    print("strs.size = " + to_string(strs.size));
    for(uint i = 0; i < strs.size; ++i)
    {
        print("strs[" + to_string(i) + "] = " + strs[i]);
    }
}

void string_array()
{
    print("check string array");
    array<string> strs = {"hello", "world"};
    print("string array created");
    assert(strs.size == 2);
    print("strs.front = " + strs.front);
    print("strs[1] = " + strs[1]);
    print_string_array(strs);

    strs.push_back("array");
    print("added new element");
    print("strs.back = " + strs.back);
    print_string_array(strs);
    assert(strs.size == 3);
    assert(strs[0] == "hello");
    assert(strs[1] == "world");
    assert(strs[2] == "array");
}

void main()
{
    array<int> ia = {1, 2, 3, 4};

    assert(ia[0] == 1);
    ia[0] = 0;
    assert(ia.front == 0);
    assert(ia == {0, 2, 3, 4});
    assert(ia.size == 4);
    print(to_string(ia[1]));

    ia.push_back(5);
    assert(ia.size == 5);
    assert(ia.back == 5);
    assert(ia == {0, 2, 3, 4, 5});

    my_pair_array();
    my_pair_ref_array();
    string_array();
}
