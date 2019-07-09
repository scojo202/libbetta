#include <math.h>
#include "data/b-data-simple.h"
#include "data/b-ring.h"
#include "data/b-linear-range.h"

#define N 1000

static void
test_simple_scalar(void)
{
  g_autoptr(BValScalar) sv = B_VAL_SCALAR(b_val_scalar_new (1.0));
  gint64 ts = b_data_get_timestamp(B_DATA(sv));
  g_assert_cmpfloat (1.0, ==, b_scalar_get_value (B_SCALAR(sv)));
  g_assert_cmpint (0, ==, b_data_get_n_dimensions (B_DATA(sv)));
  g_assert_cmpint (1, ==, b_data_get_n_values (B_DATA(sv)));
  g_assert_true(b_data_has_value(B_DATA(sv)));

  g_autofree gchar *str = b_scalar_get_str(B_SCALAR(sv),"%1.1f");

  g_assert_cmpstr("1.0",==, str);
  gint64 ts2 = b_data_get_timestamp(B_DATA(sv));
  g_assert_true(ts == ts2);
}

static void
test_simple_vector_new(void)
{
  double *vals = g_malloc(sizeof(double)*N);
  int i;
  for(i=0;i<N;i++) {
    vals[i]=(double) i;
  }
  g_autoptr(BValVector) vv = B_VAL_VECTOR(b_val_vector_new (vals,N,g_free));
  g_assert_cmpfloat (1.0, ==, b_vector_get_value (B_VECTOR(vv),1));
  g_assert_cmpint (1, ==, b_data_get_n_dimensions (B_DATA(vv)));
  g_assert_cmpint (N, ==, b_data_get_n_values (B_DATA(vv)));
  g_assert_true(b_data_has_value(B_DATA(vv)));
  double *vals2 = b_val_vector_get_array (vv);
  g_assert_true(vals2==vals);
  g_assert_cmpmem(vals,sizeof(double)*N,vals2,sizeof(double)*N);

  g_assert_cmpint (N, ==, b_vector_get_len (B_VECTOR(vv)));
  const double *vals3 = b_vector_get_values(B_VECTOR(vv));
  g_assert_cmpmem(vals,sizeof(double)*N,vals3,sizeof(double)*N);

  for(i=0;i<N;i++) {
    g_assert_cmpfloat(b_vector_get_value(B_VECTOR(vv),i),==,(double)i);
  }

  g_autofree gchar *str = b_vector_get_str(B_VECTOR(vv),89,"%1.1f");
  g_assert_cmpstr(str,==,"89.0");
  g_assert_true(b_vector_is_varying_uniformly(B_VECTOR(vv)));
  double mn, mx;
  b_vector_get_minmax(B_VECTOR(vv),&mn,&mx);
  g_assert_cmpfloat(mn, ==, 0.0);
  g_assert_cmpfloat(mx, ==, 1.0*(N-1));
}

static void
test_simple_vector_alloc(void)
{
  g_autoptr(BValVector) vv = B_VAL_VECTOR(b_val_vector_new_alloc (100));
  double *vals = b_val_vector_get_array(vv);
  int i;
  for(i=0;i<100;i++) {
    vals[i]=(double) i;
  }
  g_assert_cmpfloat (1.0, ==, b_vector_get_value (B_VECTOR(vv),1));
  g_assert_cmpint (1, ==, b_data_get_n_dimensions (B_DATA(vv)));
  g_assert_cmpint (100, ==, b_data_get_n_values (B_DATA(vv)));
  g_assert_true(b_data_has_value(B_DATA(vv)));
  const double *vals2 = b_vector_get_values (B_VECTOR(vv));
  g_assert_true(vals2==vals);
  g_assert_cmpmem(vals,sizeof(double)*100,vals2,sizeof(double)*100);

  for(i=0;i<100;i++) {
    g_assert_cmpfloat(b_vector_get_value(B_VECTOR(vv),i),==,(double)i);
  }
  g_autofree gchar *str = b_vector_get_str(B_VECTOR(vv),89,"%1.1f");
  g_assert_cmpstr(str,==,"89.0");
  g_assert_true(b_vector_is_varying_uniformly(B_VECTOR(vv)));
  double mn, mx;
  b_vector_get_minmax(B_VECTOR(vv),&mn,&mx);
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
  g_autoptr(BValVector) vv = B_VAL_VECTOR(b_val_vector_new_copy (vals0,100));
  double *vals = b_val_vector_get_array(vv);
  for(i=0;i<100;i++) {
    vals[i]=(double) i;
  }
  g_assert_cmpfloat (1.0, ==, b_vector_get_value (B_VECTOR(vv),1));
  g_assert_cmpint (1, ==, b_data_get_n_dimensions (B_DATA(vv)));
  g_assert_cmpint (100, ==, b_data_get_n_values (B_DATA(vv)));
  g_assert_true(b_data_has_value(B_DATA(vv)));
  double *vals2 = b_val_vector_get_array (vv);
  g_assert_false(vals0==vals);
  g_assert_cmpmem(vals,sizeof(double)*100,vals2,sizeof(double)*100);

  for(i=0;i<100;i++) {
    g_assert_cmpfloat(b_vector_get_value(B_VECTOR(vv),i),==,(double)i);
  }

  g_autofree gchar *str = b_vector_get_str(B_VECTOR(vv),89,"%1.1f");
  g_assert_cmpstr(str,==,"89.0");
  g_assert_true(b_vector_is_varying_uniformly(B_VECTOR(vv)));
  double mn, mx;
  b_vector_get_minmax(B_VECTOR(vv),&mn,&mx);
  g_assert_cmpfloat(mn, ==, 0.0);
  g_assert_cmpfloat(mx, ==, 99.0);

  g_free(vals0);
}

static void
test_ring_vector(void)
{
  BRingVector *r = B_RING_VECTOR(b_ring_vector_new(100, 0, FALSE));
  g_assert_cmpuint(0, ==, b_vector_get_len(B_VECTOR(r)));
  int i;
  for(i=0;i<10;i++) {
    b_ring_vector_append(r,(double)i);
  }
  g_assert_cmpuint(10, ==, b_vector_get_len(B_VECTOR(r)));
  for(i=0;i<10;i++) {
    g_assert_cmpfloat((double)i, ==, b_vector_get_value(B_VECTOR(r),i));
  }
  g_assert_true(b_vector_is_varying_uniformly(B_VECTOR(r)));
  b_ring_vector_set_length(r,5);
  g_assert_cmpuint(5, ==, b_vector_get_len(B_VECTOR(r)));

  b_ring_vector_set_max_length(r,3);
  g_assert_cmpuint(3, ==, b_vector_get_len(B_VECTOR(r)));
  g_object_unref(r);
}

static void
test_ring_matrix(void)
{
  BRingMatrix *r = B_RING_MATRIX(b_ring_matrix_new(10,100, 0, FALSE));
  g_assert_cmpuint(0, ==, b_matrix_get_rows(B_MATRIX(r)));
  g_assert_cmpuint(10, ==, b_matrix_get_columns(B_MATRIX(r)));
  int i;
  double vals[10];
  for(i=0;i<10;i++) {
    vals[i]=(double)i;
  }
  for(i=0;i<10;i++) {
    b_ring_matrix_append(r,vals,10);
  }
  g_assert_cmpuint(10, ==, b_matrix_get_rows(B_MATRIX(r)));
  for(i=0;i<10;i++) {
    g_assert_cmpfloat((double)i, ==, b_matrix_get_value(B_MATRIX(r),0,i));
  }
  b_ring_matrix_set_rows(r,5);
  g_assert_cmpuint(5, ==, b_matrix_get_rows(B_MATRIX(r)));
  g_object_unref(r);
}

static void
test_range_vectors(void)
{
  BLinearRangeVector *r = B_LINEAR_RANGE_VECTOR(b_linear_range_vector_new(0.0,1.0,10));
  g_assert_cmpuint(10, ==, b_vector_get_len(B_VECTOR(r)));
  g_assert_cmpfloat(0.0, ==, b_linear_range_vector_get_v0(r));
  g_assert_cmpfloat(1.0, ==, b_linear_range_vector_get_dv(r));
  int i;
  for(i=0;i<10;i++) {
    g_assert_cmpfloat(0.0+1.0*i, ==, b_vector_get_value(B_VECTOR(r),i));
  }
  g_autofree gchar *str = b_vector_get_str(B_VECTOR(r),8,"%1.1f");
  g_assert_cmpstr(str,==,"8.0");
  g_assert_true(b_vector_is_varying_uniformly(B_VECTOR(r)));
  double mn, mx;
  b_vector_get_minmax(B_VECTOR(r),&mn,&mx);
  g_assert_cmpfloat(mn, ==, 0.0);
  g_assert_cmpfloat(mx, ==, 9.0);

  BFourierLinearRangeVector *f = B_FOURIER_LINEAR_RANGE_VECTOR(b_fourier_linear_range_vector_new(r));
  g_assert_cmpuint(10/2+1,==,b_vector_get_len(B_VECTOR(f)));
  g_assert_cmpfloat(0.0, ==, b_vector_get_value(B_VECTOR(f),0));
  g_assert_true(b_vector_is_varying_uniformly(B_VECTOR(f)));
  g_object_unref(f);
}

int
main (int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/YData/simple/scalar",test_simple_scalar);
  g_test_add_func("/YData/simple/vector_new",test_simple_vector_new);
  g_test_add_func("/YData/simple/vector_alloc",test_simple_vector_alloc);
  g_test_add_func("/YData/simple/vector_copy",test_simple_vector_copy);
  g_test_add_func("/BData/ring/vector",test_ring_vector);
  g_test_add_func("/BData/ring/matrix",test_ring_matrix);
  g_test_add_func("/BData/range",test_range_vectors);
  return g_test_run();
}
