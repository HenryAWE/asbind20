void main()
{
    assert(min(1, 2) == 1);
    assert(max(1, 2) == 2);
    assert(abs(1) == 1);
    assert(abs(-1) == 1);

    assert(close_to(PI_f, 3.14f, 0.01f));
    assert(close_to(E_f, 2.71f, 0.01f));

    assert(close_to(abs(1.5f), 1.5f));
    assert(close_to(abs(-1.5f), 1.5f));

    assert(close_to(ceil(1.6f), 2.0f));
    assert(close_to(floor(1.6f), 1.0f));
    assert(close_to(round(1.6f), 2.0f));
    assert(close_to(round(1.4f), 1.0f));

    assert(close_to(log(E_f), 1.0f, 0.001f));

    assert(isfinite(1.0f));
    assert(isinf(INFINITY_f));
    assert(isnan(acos(2.0f)));

    assert(close_to(hypot(3.0f, 4.0f), 5.0f, 0.001f));
}
