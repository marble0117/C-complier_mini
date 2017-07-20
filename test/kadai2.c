int printf ();

int mrn (int n)
{
    if (n < 0 || n == 0)
        return 0;
    else
        return 10 * mrn (n - 1) + n;
}

int main ()
{
  int i;

  i = 0;
  while (i < 11) {
    printf ("mrn(%d) = %d\n", i, mrn(i));
    i = i+1;
  }
}
