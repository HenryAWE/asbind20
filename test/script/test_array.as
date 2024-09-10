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

    print("Modifying first element");
    pairs.front = my_pair(-1, -2);
    print("pairs[0].x = " + to_string(pairs[0].x));
    print("pairs[0].y = " + to_string(pairs[0].y));
    assert(pairs[0] == my_pair(-1, -2));
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
    assert(strs == {"hello", "world", "array"});

    strs.front = "Hello";
    assert(strs[0] == "Hello");

    print("strs.capacity = " + to_string(strs.capacity));
    assert(strs.capacity >= 3);
    strs.clear();
    assert(strs.empty);
    assert(strs.capacity >= 3);

    print("""Creating strs2 == {"yes", "yes", "yes"}""");
    array<string> strs2 = array<string>(3, "yes");
    assert(strs2.size == 3);
    assert(strs2 == {"yes", "yes", "yes"});

    print("""Inserting 3 "no"s in the front of strs2""");
    strs2.insert_range(0, array<string>(3, "no"));
    print_string_array(strs2);
    assert(strs2.size == 6);
    assert(strs2.front == "no");
    assert(strs2[2] == "no");
    assert(strs2[3] == "yes");

    print("Erasing range [1, 1+4) from the strs2");
    strs2.erase(1, 4);
    print_string_array(strs2);
    assert(strs2.size == 2);
    assert(strs2.front == "no");
    assert(strs2.back == "yes");
}

void check_int_array()
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

    ia.append_range({6, 7, 8, 9}, 3);
    assert(ia.size == 8);
    assert(ia.back == 8);
    assert(ia.front == 0);

    ia.insert(1, 1);
    for(uint i = 0; i < ia.size; ++i)
    {
        print("ia[" + to_string(i) + "] = " + to_string(ia[i]));
    }
    assert(ia.size == 9);
    assert(ia.back == 8);
    assert(ia.front == 0);
    assert(ia[1] == 1);

    ia.insert_range(0, {-3, -2, -1, 0}, 3);
    assert(ia.size == 12);
    for(uint i = 0; i < ia.size; ++i)
    {
        print("ia[" + to_string(i) + "] = " + to_string(ia[i]));
    }

    ia = array<int>(4);
    assert(ia.size == 4);
    for(uint i = 0; i < ia.size; ++i)
    {
        print("ia[" + to_string(i) + "] = " + to_string(ia[i]));
        assert(ia[i] == 0);
    }

    ia = array<int>(3, -1);
    assert(ia.size == 3);
    for(uint i = 0; i < ia.size; ++i)
    {
        print("ia[" + to_string(i) + "] = " + to_string(ia[i]));
        assert(ia[i] == -1);
    }
}

void main()
{
    check_int_array();
    my_pair_array();
    my_pair_ref_array();
    string_array();
}
