export void set_indices(uniform int a[], uniform int length)
{
    foreach (i = 0 ... length)
    {
        a[i] = i;
    }
}
