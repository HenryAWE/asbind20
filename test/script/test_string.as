void main()
{
    print("test string");

    string hello = "hello";

    hello.append(' ');
    hello = hello + "world";
    assert(hello == "hello world");

    hello[0] = 32;
    hello[0];

    print(hello);
}
