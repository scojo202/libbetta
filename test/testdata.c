#include <math.h>
#include "data/y-data-simple.h"

#define N 1000

static void
test_simple_scalar(void)
{
  g_autoptr(YValScalar) sv = Y_VAL_SCALAR(y_val_scalar_new (1.0));
  g_assert_cmpfloat (1.0, ==, y_scalar_get_value (Y_SCALAR(sv)));
  g_assert_cmpint (0, ==, y_data_get_n_dimensions (Y_DATA(sv)));
  g_assert_cmpint (1, ==, y_data_get_n_values (Y_DATA(sv)));
  g_assert_true(y_data_has_value(Y_DATA(sv)));
  
  g_autofree gchar *str = y_scalar_get_str(Y_SCALAR(sv),"%1.1f");

  g_assert_cmpstr("1.0",==, str);
}

static void
test_simple_vector_new(void)
{
  double *vals = g_malloc(sizeof(double)*N);
  int i;
  for(i=0;i<N;i++) {
    vals[i]=(double) i;
  }
  g_autoptr(YValVector) vv = Y_VAL_VECTOR(y_val_vector_new (vals,N,g_free));
  g_assert_cmpfloat (1.0, ==, y_vector_get_value (Y_VECTOR(vv),1));
  g_assert_cmpint (1, ==, y_data_get_n_dimensions (Y_DATA(vv)));
  g_assert_cmpint (N, ==, y_data_get_n_values (Y_DATA(vv)));
  g_assert_true(y_data_has_value(Y_DATA(vv)));
  double *vals2 = y_val_vector_get_array (vv);
  g_assert_true(vals2==vals);
  g_assert_cmpmem(vals,sizeof(double)*N,vals2,sizeof(double)*N);

  g_assert_cmpint (N, ==, y_vector_get_len (Y_VECTOR(vv)));
  const double *vals3 = y_vector_get_values(Y_VECTOR(vv));
  g_assert_cmpmem(vals,sizeof(double)*N,vals3,sizeof(double)*N);

  for(i=0;i<N;i++) {
    g_assert_cmpfloat(y_vector_get_value(Y_VECTOR(vv),i),==,(double)i);
  }
  
  g_autofree gchar *str = y_vector_get_str(Y_VECTOR(vv),89,"%1.1f");
  g_assert_cmpstr(str,==,"89.0");
  g_assert_true(y_vector_is_varying_uniformly(Y_VECTOR(vv)));
  double mn, mx;
  y_vector_get_minmax(Y_VECTOR(vv),&mn,&mx);
  g_assert_cmpfloat(mn, ==, 0.0);
  g_assert_cmpfloat(mx, ==, 1.0*(N-1));
}

static void
test_simple_vector_alloc(void)
{
  g_autoptr(YValVector) vv = Y_VAL_VECTOR(y_val_vector_new_alloc (100));
  double *vals = y_val_vector_get_array(vv);
  int i;
  for(i=0;i<100;i++) {
    vals[i]=(double) i;
  }
  g_assert_cmpfloat (1.0, ==, y_vector_get_value (Y_VECTOR(vv),1));
  g_assert_cmpint (1, ==, y_data_get_n_dimensions (Y_DATA(vv)));
  g_assert_cmpint (100, ==, y_data_get_n_values (Y_DATA(vv)));
  g_assert_true(y_data_has_value(Y_DATA(vv)));
  const double *vals2 = y_vector_get_values (Y_VECTOR(vv));
  g_assert_true(vals2==vals);
  g_assert_cmpmem(vals,sizeof(double)*100,vals2,sizeof(double)*100);

  for(i=0;i<100;i++) {
    g_assert_cmpfloat(y_vector_get_value(Y_VECTOR(vv),i),==,(double)i);
  }
  g_autofree gchar *str = y_vector_get_str(Y_VECTOR(vv),89,"%1.1f");
  g_assert_cmpstr(str,==,"89.0");
  g_assert_true(y_vector_is_varying_uniformly(Y_VECTOR(vv)));
  double mn, mx;
  y_vector_get_minmax(Y_VECTOR(vv),&mn,&mx);
  g_assert_cmpfloat(mn, ==, 0.0);
  g_assert_cmpfloat(mx, ==, 99.0);
}

static void
test_simple_vector_copy(void)
{
  double *vals0 = g_malloc(sizeof(double)*100);
  int i;
  for(i=0;i<100;i++) {
    vals0[i]=(double) i;
  }
  g_autoptr(YValVector) vv = Y_VAL_VECTOR(y_val_vector_new_copy (vals0,100));
  double *vals = y_val_vector_get_array(vv);
  for(i=0;i<100;i++) {
    vals[i]=(double) i;
  }
  g_assert_cmpfloat (1.0, ==, y_vector_get_value (Y_VECTOR(vv),1));
  g_assert_cmpint (1, ==, y_data_get_n_dimensions (Y_DATA(vv)));
  g_assert_cmpint (100, ==, y_data_get_n_values (Y_DATA(vv)));
  g_assert_true(y_data_has_value(Y_DATA(vv)));
  double *vals2 = y_val_vector_get_array (vv);
  g_assert_false(vals0==vals);
  g_assert_cmpmem(vals,sizeof(double)*100,vals2,sizeof(double)*100);

  for(i=0;i<100;i++) {
    g_assert_cmpfloat(y_vector_get_value(Y_VECTOR(vv),i),==,(double)i);
  }
  
  g_autofree gchar *str = y_vector_get_str(Y_VECTOR(vv),89,"%1.1f");
  g_assert_cmpstr(str,==,"89.0");
  g_assert_true(y_vector_is_varying_uniformly(Y_VECTOR(vv)));
  double mn, mx;
  y_vector_get_minmax(Y_VECTOR(vv),&mn,&mx);
  g_assert_cmpfloat(mn, ==, 0.0);
  g_assert_cmpfloat(mx, ==, 99.0);
  
  g_free(vals0);
}

int
main (int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/YData/simple/scalar",test_simple_scalar);
  g_test_add_func("/YData/simple/vector_new",test_simple_vector_new);
  g_test_add_func("/YData/simple/vector_alloc",test_simple_vector_alloc);
  g_test_add_func("/YData/simple/vector_copy",test_simple_vector_copy);
  return g_test_run();
}
