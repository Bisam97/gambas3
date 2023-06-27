/***************************************************************************

	c_float_array.c

	gb.gsl component

	(c) Benoît Minisini <benoit.minisini@gambas-basic.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 1, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
	MA 02110-1301, USA.

***************************************************************************/

#define __C_FLOAT_ARRAY_C

#include "c_float_array.h"
#include <gsl/gsl_sort.h>
#include <gsl/gsl_statistics.h>

#define THIS ((CARRAY *)_object)
#define DATA ((double *)(THIS->array.data))
#define COUNT (THIS->array.count)

//-------------------------------------------------------------------------

static bool check_array(void *array, int count, void **data)
{
	if (!array)
	{
		*data = NULL;
		return FALSE;
	}
	
	if (GB.CheckObject(array))
		return TRUE;
	
	if (((CARRAY *)array)->array.count != count)
	{
		GB.Error("Incorrect array size");
		return TRUE;
	}
	
	*data = ((CARRAY *)array)->array.data;
	return FALSE;
}

BEGIN_METHOD(FloatArrayStat_Mean, GB_OBJECT weight)

	double *weight;
	
	if (check_array(VARGOPT(weight, NULL), COUNT, POINTER(&weight)))
		return;

	if (weight)
		GB.ReturnFloat(gsl_stats_wmean(weight, 1, DATA, 1, COUNT));
	else
		GB.ReturnFloat(gsl_stats_mean(DATA, 1, COUNT));

END_METHOD

BEGIN_METHOD(FloatArrayStat_Variance, GB_OBJECT weight; GB_FLOAT mean; GB_BOOLEAN fixed)

	double *weight;
	double mean;
	
	if (check_array(VARGOPT(weight, NULL), COUNT, POINTER(&weight)))
		return;
	
	if (weight)
	{
		mean = VARGOPT(mean, gsl_stats_wmean(weight, 1, DATA, 1, COUNT));
		
		if (VARGOPT(fixed, FALSE))
			GB.ReturnFloat(gsl_stats_wvariance_m(weight, 1, DATA, 1, COUNT, mean));
		else
			GB.ReturnFloat(gsl_stats_wvariance_with_fixed_mean(weight, 1, DATA, 1, COUNT, mean));
	}
	else
	{
		mean = VARGOPT(mean, gsl_stats_mean(DATA, 1, COUNT));
		
		if (VARGOPT(fixed, FALSE))
			GB.ReturnFloat(gsl_stats_variance_m(DATA, 1, COUNT, mean));
		else
			GB.ReturnFloat(gsl_stats_variance_with_fixed_mean(DATA, 1, COUNT, mean));
	}

END_METHOD

BEGIN_METHOD(FloatArrayStat_StdDev, GB_OBJECT weight; GB_FLOAT mean; GB_BOOLEAN fixed)

	double *weight;
	double mean;
	
	if (check_array(VARGOPT(weight, NULL), COUNT, POINTER(&weight)))
		return;
	
	if (weight)
	{
		mean = VARGOPT(mean, gsl_stats_wmean(weight, 1, DATA, 1, COUNT));
		
		if (VARGOPT(fixed, FALSE))
			GB.ReturnFloat(gsl_stats_wsd_m(weight, 1, DATA, 1, COUNT, mean));
		else
			GB.ReturnFloat(gsl_stats_wsd_with_fixed_mean(weight, 1, DATA, 1, COUNT, mean));
	}
	else
	{
		mean = VARGOPT(mean, gsl_stats_mean(DATA, 1, COUNT));
		
		if (VARGOPT(fixed, FALSE))
			GB.ReturnFloat(gsl_stats_sd_m(DATA, 1, COUNT, mean));
		else
			GB.ReturnFloat(gsl_stats_sd_with_fixed_mean(DATA, 1, COUNT, mean));
	}

END_METHOD

BEGIN_METHOD(FloatArrayStat_Tss, GB_OBJECT weight; GB_FLOAT mean)

	double *weight;
	double mean;
	
	if (check_array(VARGOPT(weight, NULL), COUNT, POINTER(&weight)))
		return;

	if (weight)
	{
		mean = VARGOPT(mean, gsl_stats_wmean(weight, 1, DATA, 1, COUNT));
		GB.ReturnFloat(gsl_stats_wtss_m(weight, 1, DATA, 1, COUNT, mean));
	}
	else
	{
		mean = VARGOPT(mean, gsl_stats_mean(DATA, 1, COUNT));
		GB.ReturnFloat(gsl_stats_tss_m(DATA, 1, COUNT, mean));
	}

END_METHOD

BEGIN_METHOD(FloatArrayStat_AbsDev,GB_OBJECT weight; GB_FLOAT mean)

	double *weight;
	double mean;
	
	if (check_array(VARGOPT(weight, NULL), COUNT, POINTER(&weight)))
		return;

	if (weight)
	{
		mean = VARGOPT(mean, gsl_stats_wmean(weight, 1, DATA, 1, COUNT));
		GB.ReturnFloat(gsl_stats_wabsdev_m(weight, 1, DATA, 1, COUNT, mean));
	}
	else
	{
		mean = VARGOPT(mean, gsl_stats_mean(DATA, 1, COUNT));
		GB.ReturnFloat(gsl_stats_absdev_m(DATA, 1, COUNT, mean));
	}
	
END_METHOD

BEGIN_METHOD(FloatArrayStat_Skew, GB_OBJECT weight; GB_FLOAT mean; GB_FLOAT sd)

	double *weight;
	double mean;
	double sd;
	
	if (check_array(VARGOPT(weight, NULL), COUNT, POINTER(&weight)))
		return;

	if (weight)
	{
		mean = VARGOPT(mean, gsl_stats_wmean(weight, 1, DATA, 1, COUNT));
		sd = VARGOPT(sd, gsl_stats_wsd_m(weight, 1, DATA, 1, COUNT, mean));
		GB.ReturnFloat(gsl_stats_wskew_m_sd(weight, 1, DATA, 1, COUNT, mean, sd));
	}
	else
	{
		mean = VARGOPT(mean, gsl_stats_mean(DATA, 1, COUNT));
		sd = VARGOPT(sd, gsl_stats_sd_m(DATA, 1, COUNT, mean));
		GB.ReturnFloat(gsl_stats_skew_m_sd(DATA, 1, COUNT, mean, sd));
	}

END_METHOD

BEGIN_METHOD(FloatArrayStat_Kurtosis, GB_OBJECT weight; GB_FLOAT mean; GB_FLOAT sd)

	double *weight;
	double mean;
	double sd;
	
	if (check_array(VARGOPT(weight, NULL), COUNT, POINTER(&weight)))
		return;

	if (weight)
	{
		mean = VARGOPT(mean, gsl_stats_wmean(weight, 1, DATA, 1, COUNT));
		sd = VARGOPT(sd, gsl_stats_wsd_m(weight, 1, DATA, 1, COUNT, mean));
		GB.ReturnFloat(gsl_stats_wkurtosis_m_sd(weight, 1, DATA, 1, COUNT, mean, sd));
	}
	else
	{
		mean = VARGOPT(mean, gsl_stats_mean(DATA, 1, COUNT));
		sd = VARGOPT(sd, gsl_stats_sd_m(DATA, 1, COUNT, mean));
		GB.ReturnFloat(gsl_stats_kurtosis_m_sd(DATA, 1, COUNT, mean, sd));
	}

END_METHOD

BEGIN_METHOD(FloatArrayStat_AutoCorrelation, GB_FLOAT mean)

	double mean = VARGOPT(mean, gsl_stats_mean(DATA, 1, COUNT));
	GB.ReturnFloat(gsl_stats_lag1_autocorrelation_m(DATA, 1, COUNT, mean));

END_METHOD

BEGIN_METHOD(FloatArrayStat_Covariance, GB_OBJECT other; GB_FLOAT mean; GB_FLOAT mean_other)

	double mean = VARGOPT(mean, gsl_stats_mean(DATA, 1, COUNT));
	double mean_other;
	double *other_data;
	
	if (check_array(VARGOPT(other, NULL), COUNT, POINTER(&other_data)))
		return;
	
	mean_other = VARGOPT(mean_other, gsl_stats_mean(other_data, 1, COUNT));
	
	GB.ReturnFloat(gsl_stats_covariance_m(DATA, 1, other_data, 1, COUNT, mean, mean_other));

END_METHOD

BEGIN_METHOD(FloatArrayStat_Correlation, GB_OBJECT other)

	double *other_data;
	
	if (check_array(VARGOPT(other, NULL), COUNT, POINTER(&other_data)))
		return;
	
	GB.ReturnFloat(gsl_stats_correlation(DATA, 1, other_data, 1, COUNT));

END_METHOD

static double *get_sorted(CARRAY *_object, bool sorted)
{
	double *data;
	int count = THIS->array.count;
	
	if (!sorted && count)
	{
		GB.Alloc(POINTER(&data), sizeof(double) * count);
		memcpy(data, THIS->array.data, sizeof(double) * count);
		gsl_sort(data, 1, count);
	}
	else
		data = THIS->array.data;
	
	return data;
}

static void free_sorted(CARRAY *_object, double *data)
{
	if (data != THIS->array.data)
		GB.Free(POINTER(&data));
}

BEGIN_METHOD(FloatArrayStat_Median, GB_BOOLEAN sorted)

	double *data = get_sorted(THIS, VARGOPT(sorted, FALSE));
	GB.ReturnFloat(gsl_stats_median_from_sorted_data(data, 1, COUNT));
	free_sorted(THIS, data);

END_METHOD

BEGIN_METHOD(FloatArrayStat_Quantile, GB_FLOAT quantile; GB_BOOLEAN sorted)

	double *data = get_sorted(THIS, VARGOPT(sorted, FALSE));
	GB.ReturnFloat(gsl_stats_quantile_from_sorted_data(data, 1, COUNT, VARG(quantile)));
	free_sorted(THIS, data);

END_METHOD

BEGIN_METHOD(FloatArrayStat_TrimmedMean, GB_FLOAT trim; GB_BOOLEAN sorted)

	double *data = get_sorted(THIS, VARGOPT(sorted, FALSE));
	GB.ReturnFloat(gsl_stats_trmean_from_sorted_data(VARG(trim), data, 1, COUNT));
	free_sorted(THIS, data);

END_METHOD

BEGIN_METHOD(FloatArrayStat_Gastwirth, GB_FLOAT sorted)

	double *data = get_sorted(THIS, VARGOPT(sorted, FALSE));
	GB.ReturnFloat(gsl_stats_gastwirth_from_sorted_data(data, 1, COUNT));
	free_sorted(THIS, data);

END_METHOD

BEGIN_METHOD_VOID(FloatArrayStat_MinValue)

	GB.ReturnFloat(gsl_stats_min(DATA, 1, COUNT));

END_METHOD

BEGIN_METHOD_VOID(FloatArrayStat_MaxValue)

	GB.ReturnFloat(gsl_stats_max(DATA, 1, COUNT));

END_METHOD

static void *get_buffer(CARRAY *_object, void *array, size_t size, int count)
{
	void *buffer;
	
	if (check_array(array, count, POINTER(&buffer)))
		return NULL;
	
	if (!buffer)
		GB.Alloc(POINTER(&buffer), size * count);
	
	return buffer;
}

static void free_buffer(void *buffer, void *array)
{
	if (!array)
		GB.Free(POINTER(&buffer));
}

BEGIN_METHOD(FloatArrayStat_Mad, GB_BOOLEAN bias; GB_OBJECT buffer)

	double *buffer = get_buffer(THIS, VARGOPT(buffer, NULL), sizeof(double), COUNT);
	
	if (VARGOPT(bias, FALSE))
		GB.ReturnFloat(gsl_stats_mad0(DATA, 1, COUNT, buffer));
	else
		GB.ReturnFloat(gsl_stats_mad(DATA, 1, COUNT, buffer));
	
	free_buffer(buffer, VARGOPT(buffer, NULL));

END_METHOD

BEGIN_METHOD(FloatArrayStat_Sn, GB_BOOLEAN sorted; GB_BOOLEAN bias; GB_OBJECT buffer)

	double *data = get_sorted(THIS, VARGOPT(sorted, FALSE));
	double *buffer = get_buffer(THIS, VARGOPT(buffer, NULL), sizeof(double), COUNT);

	if (VARGOPT(bias, FALSE))
		GB.ReturnFloat(gsl_stats_Sn0_from_sorted_data(data, 1, COUNT, buffer));
	else
		GB.ReturnFloat(gsl_stats_Sn_from_sorted_data(data, 1, COUNT, buffer));
	
	free_buffer(buffer, VARGOPT(buffer, NULL));
	free_sorted(THIS, data);

END_METHOD

BEGIN_METHOD(FloatArrayStat_Qn, GB_BOOLEAN sorted; GB_BOOLEAN bias; GB_OBJECT buffer; GB_OBJECT buffer_int)

	double *data = get_sorted(THIS, VARGOPT(sorted, FALSE));
	double *buffer = get_buffer(THIS, VARGOPT(buffer, NULL), sizeof(double), 3 * COUNT);
	int *buffer_int = get_buffer(THIS, VARGOPT(buffer_int, NULL), sizeof(int), 5 * COUNT);

	if (VARGOPT(bias, FALSE))
		GB.ReturnFloat(gsl_stats_Qn0_from_sorted_data(data, 1, COUNT, buffer, buffer_int));
	else
		GB.ReturnFloat(gsl_stats_Qn_from_sorted_data(data, 1, COUNT, buffer, buffer_int));
	
	free_buffer(buffer, VARGOPT(buffer, NULL));
	free_buffer(buffer_int, VARGOPT(buffer_int, NULL));
	free_sorted(THIS, data);

END_METHOD

//-------------------------------------------------------------------------

GB_DESC FloatArrayStatDesc[] =
{
	GB_DECLARE_VIRTUAL(".FloatArrayStat"),
	
	GB_METHOD("Mean", "f", FloatArrayStat_Mean, "[(Weight)Float[];]"),
	GB_METHOD("Variance", "f", FloatArrayStat_Variance, "[(Weight)Float[];(Mean)f(Fixed)b]"),
	GB_METHOD("StdDev", "f", FloatArrayStat_StdDev, "[(Weight)Float[];(Mean)f(Fixed)b]"),
	GB_METHOD("Tss", "f", FloatArrayStat_Tss, "[(Weight)Float[];(Mean)f]"),
	GB_METHOD("AbsDev", "f", FloatArrayStat_AbsDev, "[(Weight)Float[];(Mean)f]"),
	GB_METHOD("Skew", "f", FloatArrayStat_Skew, "[(Weight)Float[];(Mean)f(StdDev)f]"),
	GB_METHOD("Kurtosis", "f", FloatArrayStat_Kurtosis, "[(Weight)Float[];(Mean)f(StdDev)f]"),
	GB_METHOD("AutoCorrelation", "f", FloatArrayStat_AutoCorrelation, "[(Mean)f]"),
	GB_METHOD("Covariance", "f", FloatArrayStat_Covariance, "(Other)Float[];[(Mean)f(MeanOther)f]"),
	GB_METHOD("Correlation", "f", FloatArrayStat_Correlation, "(Other)Float[];"),
	GB_METHOD("Median", "f", FloatArrayStat_Median, "[(Sorted)b]"),
	GB_METHOD("Quantile", "f", FloatArrayStat_Quantile, "(Quantile)f[(Sorted)b]"),
	GB_METHOD("TrimmedMean", "f", FloatArrayStat_TrimmedMean, "(Trim)f[(Sorted)b]"),
	GB_METHOD("Gastwirth", "f", FloatArrayStat_Gastwirth, "[(Sorted)b]"),
	GB_METHOD("Min", "f", FloatArrayStat_MinValue, NULL),
	GB_METHOD("Max", "f", FloatArrayStat_MaxValue, NULL),
	GB_METHOD("Mad", "f", FloatArrayStat_Mad, "[(Bias)b(Buffer)Float[];]"),
	GB_METHOD("Sn", "f", FloatArrayStat_Sn, "[(Sorted)b(Bias)b(Buffer)Float[];]"),
	GB_METHOD("Qn", "f", FloatArrayStat_Qn, "[(Sorted)b(Bias)b(Buffer)Float[];(BufferInt)Integer[];]"),
	
	GB_END_DECLARE
};

GB_DESC FloatArrayDesc[] =
{
	GB_DECLARE("Float[]", sizeof(CARRAY)),
	
	GB_PROPERTY_SELF("Stat", ".FloatArrayStat"),
	
	GB_END_DECLARE
};
