#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h>

#define STR_MAX 256

static double
get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

static void
gen_string_from_integer(char *str_out, int num)
{
    snprintf(str_out, STR_MAX, "mystring-%d", num);
}

static void
put_mark (void)
{
    fputc ('#', stdout);
    fflush (stdout);
}

static void
maybe_put_mark (int n_ops)
{
    if (n_ops > 0 && !(n_ops % 1000))
        put_mark ();
}

int main(int argc, char ** argv)
{
    int num_keys = atoi(argv[1]);
    unsigned int i, value = 0x1;
    unsigned int tkey;
    char str [STR_MAX];

    if(argc <= 2)
        return 1;

    SETUP

    double before = get_time();

    if(!strcmp(argv[2], "sequential"))
    {
        for(i = 0; i < num_keys; i++)
        {
            INSERT_INT_INTO_HASH(i, value);
            maybe_put_mark (i);
        }
    }

    else if(!strcmp(argv[2], "spaced"))
    {
        for(i = 0; i < num_keys; i++)
        {
            INSERT_INT_INTO_HASH((uint64_t) i * 256, value);
            maybe_put_mark (i);
        }
    }

    else if(!strcmp(argv[2], "random"))
    {
        srandom(1); // for a fair/deterministic comparison
        for(i = 0; i < num_keys; i++)
        {
            INSERT_INT_INTO_HASH(random() & 0x0fffffff, value);
            maybe_put_mark (i);
        }
    }

    else if(!strcmp(argv[2], "delete"))
    {
        for(i = 0; i < num_keys; i++)
        {
            INSERT_INT_INTO_HASH(i, value);
            maybe_put_mark (i);
        }

        before = get_time();
        for(i = 0; i < num_keys; i++)
        {
            DELETE_INT_FROM_HASH(i);
            maybe_put_mark (i);
        }

        /* Required to make some implementations release memory */
        INSERT_INT_INTO_HASH(1, value);

        /* Release as much heap memory as possible */
        malloc_trim (0);

        /* Sleep for a bit so monitoring process can pick up the final size */
        usleep (200000);
    }

    else if(!strcmp(argv[2], "aging"))
    {
        srandom(1); // for a fair/deterministic comparison

        /* First populate the table */
        for(i = 0; i < num_keys; i++)
        {
            tkey = (int)random() % num_keys;
            INSERT_INT_INTO_HASH(tkey, value);
            maybe_put_mark (i);
        }

        before = get_time();

        /* Randomly insert and delete keys for a bit */
        for(i = 0; i < num_keys; i++)
        {
            tkey = (int)random() % num_keys;
            DELETE_INT_FROM_HASH(tkey);

            tkey = (int)random() % num_keys;
            INSERT_INT_INTO_HASH(tkey, value);
            maybe_put_mark (i);
        }
    }

    else if(!strcmp(argv[2], "lookups"))
    {
        srandom(1); // for a fair/deterministic comparison

        for(i = 0; i < num_keys; i++)
        {
            INSERT_INT_INTO_HASH((int)random() % num_keys, value);
            LOOKUP_INT ((int)random() % num_keys);
            LOOKUP_INT ((int)random() % num_keys);
            LOOKUP_INT ((int)random() % num_keys);
            LOOKUP_INT ((int)random() % num_keys);
            LOOKUP_INT ((int)random() % num_keys);
            maybe_put_mark (i);
        }
    }

    else if(!strcmp(argv[2], "sequentialstring"))
    {
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, i);
            INSERT_STR_INTO_HASH (str, value);
            maybe_put_mark (i);
        }
    }

    else if(!strcmp(argv[2], "randomstring"))
    {
        srandom(1); // for a fair/deterministic comparison
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, random ());
            INSERT_STR_INTO_HASH (str, value);
            maybe_put_mark (i);
        }
    }

    else if(!strcmp(argv[2], "deletestring"))
    {
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, i);
            INSERT_STR_INTO_HASH (str, value);
            maybe_put_mark (i);
        }

        before = get_time();
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, i);
            DELETE_STR_FROM_HASH (str);
            maybe_put_mark (i);
        }

        /* Release as much heap memory as possible */
        malloc_trim (0);

        /* Sleep for a bit so monitoring process can pick up the final size */
        usleep (200000);
    }

    else if(!strcmp(argv[2], "agingstring"))
    {
        srandom(1); // for a fair/deterministic comparison

        /* First populate the table */
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, random () % num_keys);
            INSERT_STR_INTO_HASH (str, value);
            maybe_put_mark (i);
        }

        before = get_time();

        /* Randomly insert and delete keys for a bit */
        for(i = 0; i < num_keys; i++)
        {
            gen_string_from_integer (str, random() % num_keys);
            DELETE_STR_FROM_HASH (str);

            gen_string_from_integer (str, random() % num_keys);
            INSERT_STR_INTO_HASH (str, value);

            maybe_put_mark (i);
        }
    }

    else if(!strcmp(argv[2], "small"))
    {
        int keys_per_table = num_keys / 10000;
        int j;

        srandom(1); // for a fair/deterministic comparison

        for (j = 0; j < 10000; j++)
        {
            /* First populate the table */
            for(i = 0; i < keys_per_table; i++)
            {
                INSERT_INT_INTO_HASH((int)random() % keys_per_table, value);
                maybe_put_mark (i);
            }

            /* Randomly insert and delete keys for a bit */
            for(i = 0; i < keys_per_table * 4; i++)
            {
                DELETE_INT_FROM_HASH((int)random() % keys_per_table);
                INSERT_INT_INTO_HASH((int)random() % keys_per_table, value);
                maybe_put_mark (i);
            }

            /* Delete all keys */
            for(i = 0; i < keys_per_table; i++)
            {
                DELETE_INT_FROM_HASH(i);
                maybe_put_mark (i);
            }
        }
    }

    _exit (0);

#if 0
    double after = get_time();

    malloc_trim (0);
    printf("%f\n", after-before);
    fflush(stdout);
    sleep(1000000);
#endif
}
