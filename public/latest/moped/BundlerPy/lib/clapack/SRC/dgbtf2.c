#include "f2c.h"
#include "blaswrap.h"

/* Table of constant values */

static integer c__1 = 1;
static doublereal c_b9 = -1.;

/* Subroutine */ int dgbtf2_(integer *m, integer *n, integer *kl, integer *ku, 
	 doublereal *ab, integer *ldab, integer *ipiv, integer *info)
{
    /* System generated locals */
    integer ab_dim1, ab_offset, i__1, i__2, i__3, i__4;
    doublereal d__1;

    /* Local variables */
    integer i__, j, km, jp, ju, kv;
    extern /* Subroutine */ int dger_(integer *, integer *, doublereal *, 
	    doublereal *, integer *, doublereal *, integer *, doublereal *, 
	    integer *), dscal_(integer *, doublereal *, doublereal *, integer 
	    *), dswap_(integer *, doublereal *, integer *, doublereal *, 
	    integer *);
    extern integer idamax_(integer *, doublereal *, integer *);
    extern /* Subroutine */ int xerbla_(char *, integer *);


/*  -- LAPACK routine (version 3.1) -- */
/*     Univ. of Tennessee, Univ. of California Berkeley and NAG Ltd.. */
/*     November 2006 */

/*     .. Scalar Arguments .. */
/*     .. */
/*     .. Array Arguments .. */
/*     .. */

/*  Purpose */
/*  ======= */

/*  DGBTF2 computes an LU factorization of a real m-by-n band matrix A */
/*  using partial pivoting with row interchanges. */

/*  This is the unblocked version of the algorithm, calling Level 2 BLAS. */

/*  Arguments */
/*  ========= */

/*  M       (input) INTEGER */
/*          The number of rows of the matrix A.  M >= 0. */

/*  N       (input) INTEGER */
/*          The number of columns of the matrix A.  N >= 0. */

/*  KL      (input) INTEGER */
/*          The number of subdiagonals within the band of A.  KL >= 0. */

/*  KU      (input) INTEGER */
/*          The number of superdiagonals within the band of A.  KU >= 0. */

/*  AB      (input/output) DOUBLE PRECISION array, dimension (LDAB,N) */
/*          On entry, the matrix A in band storage, in rows KL+1 to */
/*          2*KL+KU+1; rows 1 to KL of the array need not be set. */
/*          The j-th column of A is stored in the j-th column of the */
/*          array AB as follows: */
/*          AB(kl+ku+1+i-j,j) = A(i,j) for max(1,j-ku)<=i<=min(m,j+kl) */

/*          On exit, details of the factorization: U is stored as an */
/*          upper triangular band matrix with KL+KU superdiagonals in */
/*          rows 1 to KL+KU+1, and the multipliers used during the */
/*          factorization are stored in rows KL+KU+2 to 2*KL+KU+1. */
/*          See below for further details. */

/*  LDAB    (input) INTEGER */
/*          The leading dimension of the array AB.  LDAB >= 2*KL+KU+1. */

/*  IPIV    (output) INTEGER array, dimension (min(M,N)) */
/*          The pivot indices; for 1 <= i <= min(M,N), row i of the */
/*          matrix was interchanged with row IPIV(i). */

/*  INFO    (output) INTEGER */
/*          = 0: successful exit */
/*          < 0: if INFO = -i, the i-th argument had an illegal value */
/*          > 0: if INFO = +i, U(i,i) is exactly zero. The factorization */
/*               has been completed, but the factor U is exactly */
/*               singular, and division by zero will occur if it is used */
/*               to solve a system of equations. */

/*  Further Details */
/*  =============== */

/*  The band storage scheme is illustrated by the following example, when */
/*  M = N = 6, KL = 2, KU = 1: */

/*  On entry:                       On exit: */

/*      *    *    *    +    +    +       *    *    *   u14  u25  u36 */
/*      *    *    +    +    +    +       *    *   u13  u24  u35  u46 */
/*      *   a12  a23  a34  a45  a56      *   u12  u23  u34  u45  u56 */
/*     a11  a22  a33  a44  a55  a66     u11  u22  u33  u44  u55  u66 */
/*     a21  a32  a43  a54  a65   *      m21  m32  m43  m54  m65   * */
/*     a31  a42  a53  a64   *    *      m31  m42  m53  m64   *    * */

/*  Array elements marked * are not used by the routine; elements marked */
/*  + need not be set on entry, but are required by the routine to store */
/*  elements of U, because of fill-in resulting from the row */
/*  interchanges. */

/*  ===================================================================== */

/*     .. Parameters .. */
/*     .. */
/*     .. Local Scalars .. */
/*     .. */
/*     .. External Functions .. */
/*     .. */
/*     .. External Subroutines .. */
/*     .. */
/*     .. Intrinsic Functions .. */
/*     .. */
/*     .. Executable Statements .. */

/*     KV is the number of superdiagonals in the factor U, allowing for */
/*     fill-in. */

    /* Parameter adjustments */
    ab_dim1 = *ldab;
    ab_offset = 1 + ab_dim1;
    ab -= ab_offset;
    --ipiv;

    /* Function Body */
    kv = *ku + *kl;

/*     Test the input parameters. */

    *info = 0;
    if (*m < 0) {
	*info = -1;
    } else if (*n < 0) {
	*info = -2;
    } else if (*kl < 0) {
	*info = -3;
    } else if (*ku < 0) {
	*info = -4;
    } else if (*ldab < *kl + kv + 1) {
	*info = -6;
    }
    if (*info != 0) {
	i__1 = -(*info);
	xerbla_("DGBTF2", &i__1);
	return 0;
    }

/*     Quick return if possible */

    if (*m == 0 || *n == 0) {
	return 0;
    }

/*     Gaussian elimination with partial pivoting */

/*     Set fill-in elements in columns KU+2 to KV to zero. */

    i__1 = min(kv,*n);
    for (j = *ku + 2; j <= i__1; ++j) {
	i__2 = *kl;
	for (i__ = kv - j + 2; i__ <= i__2; ++i__) {
	    ab[i__ + j * ab_dim1] = 0.;
/* L10: */
	}
/* L20: */
    }

/*     JU is the index of the last column affected by the current stage */
/*     of the factorization. */

    ju = 1;

    i__1 = min(*m,*n);
    for (j = 1; j <= i__1; ++j) {

/*        Set fill-in elements in column J+KV to zero. */

	if (j + kv <= *n) {
	    i__2 = *kl;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		ab[i__ + (j + kv) * ab_dim1] = 0.;
/* L30: */
	    }
	}

/*        Find pivot and test for singularity. KM is the number of */
/*        subdiagonal elements in the current column. */

/* Computing MIN */
	i__2 = *kl, i__3 = *m - j;
	km = min(i__2,i__3);
	i__2 = km + 1;
	jp = idamax_(&i__2, &ab[kv + 1 + j * ab_dim1], &c__1);
	ipiv[j] = jp + j - 1;
	if (ab[kv + jp + j * ab_dim1] != 0.) {
/* Computing MAX */
/* Computing MIN */
	    i__4 = j + *ku + jp - 1;
	    i__2 = ju, i__3 = min(i__4,*n);
	    ju = max(i__2,i__3);

/*           Apply interchange to columns J to JU. */

	    if (jp != 1) {
		i__2 = ju - j + 1;
		i__3 = *ldab - 1;
		i__4 = *ldab - 1;
		dswap_(&i__2, &ab[kv + jp + j * ab_dim1], &i__3, &ab[kv + 1 + 
			j * ab_dim1], &i__4);
	    }

	    if (km > 0) {

/*              Compute multipliers. */

		d__1 = 1. / ab[kv + 1 + j * ab_dim1];
		dscal_(&km, &d__1, &ab[kv + 2 + j * ab_dim1], &c__1);

/*              Update trailing submatrix within the band. */

		if (ju > j) {
		    i__2 = ju - j;
		    i__3 = *ldab - 1;
		    i__4 = *ldab - 1;
		    dger_(&km, &i__2, &c_b9, &ab[kv + 2 + j * ab_dim1], &c__1, 
			     &ab[kv + (j + 1) * ab_dim1], &i__3, &ab[kv + 1 + 
			    (j + 1) * ab_dim1], &i__4);
		}
	    }
	} else {

/*           If pivot is zero, set INFO to the index of the pivot */
/*           unless a zero pivot has already been found. */

	    if (*info == 0) {
		*info = j;
	    }
	}
/* L40: */
    }
    return 0;

/*     End of DGBTF2 */

} /* dgbtf2_ */
