#!/usr/bin/python

def bubblesort(tab):
	n = len(tab)
	for i in range(n - 1):
		for j in range(i + 1, n):
			if tab[j] < tab[i]:
				tab[i], tab[j] = tab[j], tab[i]

src = [212,239,2,675,132,836,123,125,219,373,
865,486,363,561,74,662,204,93,682,977,
842,495,839,797,809,799,26,27,952,907,
719,990,472,578,585,155,828,913,38,735,
351,531,18,778,44,479,415,99,12,644,
556,670,468,590,454,331,8,343,362,353,
648,625,704,63,389,906,75,845,702,69,
481,418,838,384,641,165,620,853,90,728,
491,890,679,344,20,973,985,225,745,783,
946,807,109,587,598,42,618,307,741,82,
222,327,575,103,924,129,657,790,108,102,
195,768,112,916,856,476,446,280,403,555,
120,534,122,852,761,633,126,487,751,707,
130,858,928,896,927,965,41,137,138,210,
933,385,840,306,437,806,146,673,471,775,
558,329,152,566,873,742,170,503,897,686,
160,106,356,198,954,380,166,715,744,169,
988,7,519,978,158,962,176,613,816,722,
59,635,182,616,77,185,767,187,854,84,
304,849,326,720,674,429,608,542,651,199,
105,904,267,290,829,908,736,934,411,571,
594,221,508,957,996,215,440,518,218,25,
784,484,629,703,205,159,643,611,911,422,
533,162,969,233,557,270,236,237,610,94,
443,43,580,493,801,711,241,382,245,374,
837,269,444,253,254,371,420,240,811,345,
786,377,262,263,961,892,826,439,330,251,
45,505,33,11,277,492,950,140,999,279,
592,281,478,525,15,273,819,560,288,294,
776,406,292,456,151,536,183,369,298,563,
780,795,100,336,753,515,153,743,401,309,
310,299,792,316,637,101,972,507,65,107,
104,321,322,220,324,200,500,935,328,189,
993,242,808,540,376,732,135,820,300,339,
275,266,881,57,188,844,690,537,348,740,
841,40,591,228,825,201,250,96,823,305,
445,13,772,889,56,396,428,681,655,697,
921,311,136,442,521,506,739,888,173,579,
113,649,548,821,459,161,386,387,543,392,
367,391,627,876,528,333,365,257,546,818,
617,932,900,781,570,706,559,114,408,463,
157,70,885,588,762,193,156,417,883,419,
700,393,116,692,424,192,943,226,71,509,
314,496,926,822,434,436,930,110,604,95,
805,340,184,898,111,583,92,770,68,255,
450,244,452,453,867,455,293,630,603,87,
667,769,35,409,297,458,466,652,574,274,
318,572,787,473,447,475,276,4,284,272,
708,320,154,717,705,847,29,750,727,213,
941,905,404,871,494,695,62,497,498,248,
796,782,19,814,810,313,58,317,955,462,
171,511,278,589,514,337,516,517,967,449,
855,882,243,141,83,149,372,282,774,529,
551,997,532,747,687,260,846,480,390,726,
124,895,16,523,544,568,398,547,46,549,
833,757,134,553,554,128,917,191,875,3,
939,612,216,886,564,565,325,53,464,17,
230,209,632,465,31,730,576,948,994,699,
197,485,731,399,584,998,167,416,413,0,
412,830,400,335,659,36,936,133,676,217,
142,235,97,76,909,295,606,202,247,51,
238,550,6,359,912,615,180,179,619,763,
526,405,366,623,410,599,872,229,628,992,
723,631,388,402,832,622,301,352,264,621,
640,28,642,47,287,433,54,81,467,539,
788,653,577,414,355,368,656,979,891,733,
660,953,483,286,664,665,666,915,668,669,
804,72,672,143,127,426,859,48,196,49,
231,813,147,562,425,894,524,803,88,457,
693,691,581,938,684,89,431,32,698,903,
658,922,98,834,593,868,395,573,285,709,
144,67,661,208,884,843,716,920,718,10,
663,427,512,942,724,624,975,677,694,848,
432,773,332,899,23,5,341,489,759,85,
349,347,877,600,636,470,940,131,748,749,
430,379,490,303,626,79,756,901,758,738,
937,864,78,14,163,765,850,760,609,60,
964,771,567,569,982,721,203,777,974,178,
338,552,725,545,265,971,378,117,172,375,
319,791,435,793,862,441,860,582,986,360,
857,520,802,283,779,734,678,190,291,794,
224,9,115,139,501,232,949,817,671,383,
685,289,86,824,223,64,407,827,381,61,
261,252,755,175,469,976,522,680,754,394,
370,510,214,211,634,502,37,30,968,638,
766,52,164,302,966,55,789,249,346,923,
586,861,530,863,650,50,866,150,259,798,
870,713,931,268,874,397,174,764,256,879,
959,342,364,983,714,712,119,605,984,869,
181,246,504,118,987,688,354,597,460,22,
296,423,513,488,800,538,66,357,893,438,
910,595,919,234,914,461,34,601,21,958,
812,929,80,945,729,168,737,746,963,206,
654,474,308,315,207,752,696,186,39,312,
947,121,145,73,944,323,614,689,951,645,
227,535,499,647,639,710,683,527,970,602,
960,334,24,541,596,701,477,646,815,851,
358,785,271,177,878,148,835,482,956,448,
980,981,421,925,902,880,194,258,918,989,
1,991,91,887,607,995,361,350,831,451]

for t in range(100):
	tab = src[:]
	bubblesort(tab)

print tab
