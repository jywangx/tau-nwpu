#include "taudb_api.h"
#include "libpq-fe.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

TAUDB_METRIC* taudb_query_metrics(TAUDB_CONNECTION* connection, TAUDB_TRIAL* trial) {
#ifdef TAUDB_DEBUG_DEBUG
  printf("Calling taudb_query_metrics(%p)\n", trial);
#endif
  void *res;
  int nFields;
  int i, j;

  if (trial == NULL) {
    fprintf(stderr, "Error: trial parameter null. Please provide a valid trial.\n");
    return NULL;
  }

  //if the Trial already has the data, return it.
  if (trial->metrics != NULL && trial->metric_count > 0) {
    taudb_numItems = trial->metric_count;
    return trial->metrics;
  }

  taudb_begin_transaction(connection);

  /*
   * Fetch rows from table_name, the system catalog of databases
   */
  char my_query[256];
  sprintf(my_query,"select * from metric where trial = %d", trial->id);
#ifdef TAUDB_DEBUG
  printf("Query: %s\n", my_query);
#endif
  res = taudb_execute_query(connection, my_query);

  int nRows = taudb_get_num_rows(res);
  taudb_numItems = nRows;

  TAUDB_METRIC* metrics = taudb_create_metrics(taudb_numItems);

  nFields = taudb_get_num_columns(res);

  /* the rows */
  for (i = 0; i < taudb_get_num_rows(res); i++)
  {
    TAUDB_METRIC* metric = &(metrics[i]);
    /* the columns */
    for (j = 0; j < nFields; j++) {
	  if (strcmp(taudb_get_column_name(res, j), "id") == 0) {
	    metric->id = atoi(taudb_get_value(res, i, j));
	  } else if (strcmp(taudb_get_column_name(res, j), "trial") == 0) {
	    //metric->trial = trial;
	  } else if (strcmp(taudb_get_column_name(res, j), "name") == 0) {
	    metric->name = taudb_create_and_copy_string(taudb_get_value(res,i,j));
	  } else if (strcmp(taudb_get_column_name(res, j), "derived") == 0) {
	    metric->derived = atoi(taudb_get_value(res, i, j));
	  } else {
	    printf("Error: unknown column '%s'\n", taudb_get_column_name(res, j));
	    taudb_exit_nicely(connection);
	  }
	} 
	HASH_ADD_KEYPTR(hh, metrics, metric->name, strlen(metric->name), metric);
  }

  taudb_clear_result(res);
  taudb_close_transaction(connection);

  return (metrics);
}

TAUDB_METRIC* taudb_get_metric(TAUDB_METRIC* metrics, const char* name) {
#ifdef TAUDB_DEBUG_DEBUG
  printf("Calling taudb_get_metric(%p,%s)\n", metrics, name);
#endif
  if (metrics == NULL) {
    fprintf(stderr, "Error: metric parameter null. Please provide a valid set of metrics.\n");
    return NULL;
  }
  if (name == NULL) {
    fprintf(stderr, "Error: name parameter null. Please provide a valid name.\n");
    return NULL;
  }

  TAUDB_METRIC* metric = NULL;
  HASH_FIND_STR(metrics, name, metric);
  return metric;
}

