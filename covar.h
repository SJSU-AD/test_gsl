#ifndef COVAR_H_
#define COVAR_H_

#include <stdio.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_matrix_double.h>
#include <vector>

// \TODO: trim excess comments 
// \TODO: split this into an implementation file

/*
 *  Okay, so here's what we need
 *  1) A data structure to store the data points (vector?)
 *  2) A way of packaging the data points (struct of floats?)
 *  3) A method to push new data points onto the data structure
 *  
 *  So the above is an API towards collecting data. On the ROS side,
 *  1) As new messages from the ROS topic arrives, we need the 
 *     data_structure.push() to happen
 * 
 *  Next, we need a method for generating the covariance matrix. This 
 *  should happen after the data collection period has expired. What needs 
 *  to happen when we generate the covariance matrix? 
 *  1) Convert the data_structure into a gsl_matrix
 *  2) Don't forget to allocate another gsl_matrix which will hold the 
 *     resulting covariance matrix.
 *  3) Double for-loop to generate the variance-covariance value for each
 *     element of the matrix
 */

// the covariance matrix of our odometry topic tracks six variables
// as in, think of each data point as a six-dimensional point with the 
// following axes: x, y, z, yaw, pitch, roll
#define ODOM_COVAR_MAT_VARSIZE 6

// struct odom_struct
// {
//     double x;
//     double y; 
//     double z; 
//     double yaw;
//     double pitch;
//     double roll;
// };

// Since we're traversing through the data points using a loop, maybe it'd 
// be better to use an array of doubles rather than a struct
// double * data_point = new data_point[6];
// We establish the convention here:
// index: 0  1  2  3    4      5
// axis:  x, y, z, yaw, pitch, roll

// However, from a maintainability perspective, relying on a double 
// pointer is bad - there is no way of guaranteeing the fixed six-element 
// size that we want for data point
// \TODO: create a better container (i.e. class) for each data point that 
//        has as a member method 
//        1) the square bracket operator, for fetching an element

class covar
{
    private:
        gsl_matrix *covar_mat;
        std::vector<double*> *data_points;
    
    public:
        covar()
        {
            data_points = new std::vector<double*>;
            covar_mat = gsl_matrix_alloc(ODOM_COVAR_MAT_VARSIZE, 
                                         ODOM_COVAR_MAT_VARSIZE);
        }
        
        ~covar()
        {
            delete covar_mat;
            covar_mat = 0;
            delete data_points;
            data_points = 0;
        }

        // We expect the input to be a double array with six elements
        bool insert(double* data_point)
        {
            // double * temp = new double[6];
            data_points->push_back(data_point);
            return true;
        }

        void print_dp()
        {
            for (auto i = 0; i < data_points->size(); i++)
            {
                printf("%i: %f %f %f %f %f %f\n",
                            i,
                            (*data_points)[i][0],
                            (*data_points)[i][1],
                            (*data_points)[i][2],
                            (*data_points)[i][3],
                            (*data_points)[i][4],
                            (*data_points)[i][5]);
            }
        }
        
        void print_mat()
        {
            for (auto i = 0; i < covar_mat->size1; i++)
            {
                printf("\n");
                for (auto j = 0; j < covar_mat->size2; j++)
                {
                    printf("%f", covar_mat->data[covar_mat->size2*i+j]);
                    if(j < covar_mat->size2 - 1) printf(", ");
                }
            }
            printf("\n");
        }

        bool generate_covar()
        {
            /* no point in generating a covariance matrix if we don't have 
             * the data points to do so
             */
            int i, j;
            int rsize = data_points->size();

            if(rsize <= 1) return false; 

            gsl_vector_view a, b;
            gsl_matrix *A;
            A = gsl_matrix_alloc(rsize, ODOM_COVAR_MAT_VARSIZE);
            /* the covariance matrix should already be instantiated by the 
             * class constructor
             */
            // covar_mat = gsl_matrix_alloc(ODOM_COVAR_MAT_VARSIZE, 
            //                              ODOM_COVAR_MAT_VARSIZE);

            // convert data_points into gsl_matrix A
            for ( i = 0; i < rsize; i++)
                for ( j = 0; j < ODOM_COVAR_MAT_VARSIZE; j++) 
                    gsl_matrix_set (A, i, j, (*data_points)[i][j]);

            // generate the var-covar values based on A
            // insert these values into covar_mat
            for (i = 0; i < A->size2; i++) {
                for (j = 0; j < A->size2; j++) {
                a = gsl_matrix_column (A, i);
                b = gsl_matrix_column (A, j);
                double cov = gsl_stats_covariance(a.vector.data, 
                                                    a.vector.stride,
                                                    b.vector.data, 
                                                    b.vector.stride, 
                                                    rsize);
                gsl_matrix_set (covar_mat, i, j, cov);
                }
            }
            
            return true;
        }
};

#endif // COVAR_H_