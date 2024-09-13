void check_unicode_str()
{
    print("check Unicode string");

    string hello_cn = "你好世界";

    assert(hello_cn.size == 4);
    assert(hello_cn.size_bytes == 4 * 3);
    assert(hello_cn.starts_with('你'));
    assert(hello_cn.ends_with('界'));
    assert(hello_cn.substr(2) == "世界");

    hello_cn.replace(0, 2, "向你问好！", 4);
    print(hello_cn);
    print(to_string(hello_cn.size));
    assert(hello_cn.size == 6);
    assert(hello_cn == "向你问好世界");
    hello_cn.insert(4, "，");
    print(hello_cn);
    assert(hello_cn == "向你问好，世界");
    hello_cn.erase(4);
    print(hello_cn);
    assert(hello_cn == "向你问好世界");
    hello_cn.erase(4, 2);
    print(hello_cn);
    assert(hello_cn == "向你问好");
    hello_cn.prepend('我');
    print(hello_cn);
    assert(hello_cn == "我向你问好");
}

void check_utils()
{
    assert(to_string(-1) == "-1");
    assert(to_string(uint(42)) == "42");
    assert(to_string(3.14) == "3.14");
}

void check_split()
{
    assert("a b c ".split() == {"a", "b", "c"});
    assert("a b c ".split(skip_empty:false) == {"a", "b", "c", ""});
    assert("a,b,c,".split(',') == {"a", "b", "c"});
    assert("a,b,c,".split(',', skip_empty:false) == {"a", "b", "c", ""});
}

void check_opCmp()
{
    print("check opCmp()");
    assert("aaa" < "aba");
    assert("aaa" <= "aaa");
    assert("aba" > "aaa");
    assert("aba" >= "aaa");
}

void main()
{
    print("test string");

    string hello = "hello";

    hello.append(' ');
    hello = hello + "world";
    assert(hello == "hello world");

    assert(hello.starts_with("hello"));
    assert(hello.ends_with("world"));
    assert(hello.substr(7, 2) == "or");

    hello[0] = 'H';
    print("hello[0] = " + chr(hello[0]));

    hello = hello + '!';
    print(hello);
    assert(hello == "Hello world!");
    hello = hello.remove_suffix(1);
    print(hello);
    assert(hello == "Hello world");
    hello = hello.remove_prefix(6);
    print(hello);
    assert(hello == "world");

    hello.clear();
    assert(hello.empty);

    check_unicode_str();
    check_utils();
    check_split();
    check_opCmp();
}
