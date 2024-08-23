void main()
{
    print("test string");

    string hello = "hello";

    hello.append(' ');
    hello = hello + "world";
    assert(hello == "hello world");
}
