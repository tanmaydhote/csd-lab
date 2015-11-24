#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <limits>

class Matrix {
    private:
        int *_data;
        size_t _size;
        
    public:
        Matrix(size_t);
        size_t size() { return _size; }
        void fillRandomData();
        void fillZeros();
        void display();
        static void multiply(Matrix&, Matrix&, Matrix&);
         
        //For printing
        friend std::ostream& operator<<(std::ostream&, const Matrix&);
};

Matrix::Matrix(size_t size) {
    _size = size;
    _data = new int[_size*_size];
}

void Matrix::fillZeros() {
    memset(_data, 0, _size*_size*sizeof(int));
}

void Matrix::fillRandomData() {
    for (size_t i = 0; i < _size; ++i) {
        for (size_t j = 0; j < _size; ++j) {
            _data[i*_size + j] = (rand() % 32) - 16;
        }
    }
}

void Matrix::multiply(Matrix& A, Matrix& B, Matrix& C) {
    size_t n = A._size;
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            for (size_t k = 0; k < n; ++k) {
                C._data[i*n + j] += A._data[i*n + k] * B._data[k*n + j];     
            }
        }
    }
}

std::ostream& operator<<(std::ostream& out, const Matrix &A) {
    for (size_t i = 0; i < A._size; ++i) {
        for (size_t j = 0; j < A._size; ++j) {
            out.width(6);
            out << A._data[i*A._size + j] << " ";
        }
        out << std::endl;
    }
    return out;
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        std::cout << "USAGE:- " << argv[0] << " <MATRIX_SIZE>" << std::endl;
        return EXIT_FAILURE;
    } 

    //Take matrix size as input
    int n;
    n = atoi(argv[1]);

    //Fill two matrices randomly
    srand(time(NULL));
    Matrix A(n), B(n); 
    A.fillRandomData();
    B.fillRandomData();
    
    //Multiply the two matrices
    Matrix C(n);
    C.fillZeros();
    Matrix::multiply(A, B, C);

#ifdef DEBUG
    std::cout << "Matrix A = " << std::endl << A << std::endl;
    std::cout << "Matrix B = " << std::endl << B << std::endl;
    std::cout << "Matrix C = " << std::endl << C << std::endl;
#endif

    return EXIT_SUCCESS;
}
