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

    print("Finding element");
    uint idx = pairs.find(my_pair(3, 3));
    print("Index = " + to_string(idx));
    assert(idx == 2);
    assert(!pairs.find_optional(my_pair(4, 5)).has_value);
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
    assert(strs.find_if(function(v) { return v.starts_with("w"); }) == 1);

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

    array<string> strs3 = {"aaa", "ccc", "bbb"};
    strs3.sort();
    assert(strs3 == {"aaa", "bbb", "ccc"});
    strs3.sort(0, -1, false);
    assert(strs3 == {"ccc", "bbb", "aaa"});

    print("Check contains()");
    assert(strs3.contains("ccc"));
    assert(!strs3.contains("ccc", 1));
}

funcdef void my_callback_t(const int&in);
void my_callback(const int&in val)
{
    print("callback: " + to_string(val));
    assert(val == -1);
}

class my_delegate
{
    int value = 2;
    void call(const int&in v)
    {
        int new_v = v + this.value;
        print("new_v = " + to_string(new_v));
        assert(new_v == 1);
    }
}

void check_int_array()
{
    array<int> ia = {1, 2, 3, 4};

    assert(ia[0] == 1);
    ia[0] = 0;
    assert(ia.front == 0);
    assert(ia == {0, 2, 3, 4});
    assert(ia.contains(0));
    assert(!ia.contains(1));
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

    ia.for_each(my_callback);

    my_delegate d;
    my_callback_t@ f = my_callback_t(d.call);
    ia.for_each(f);

    ia = {1, 3, 4, 6, 7, 9, 8, 5, 2};
    ia.sort();
    assert(ia == {1, 2, 3, 4, 5, 6, 7, 8, 9});
    ia.sort(0, -1, false);
    assert(ia == {9, 8, 7, 6, 5, 4, 3, 2, 1});
}

void main()
{
    check_int_array();
    my_pair_array();
    my_pair_ref_array();
    string_array();
}
