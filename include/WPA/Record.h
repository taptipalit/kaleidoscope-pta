#ifndef LOG_SVF
#define LOG_SVF

class Record {
    private:
        enum Type {
            SOLVE_COPY,
            SOLVE_GEP,
            MERGE_CYCLE
        };

        Type recordType;

        int recordId;

    public:
        Record(int id, Type recordType) {
            this->id = id;
            this->recordType = recordType;
        }

};

class ConstraintSolveRecord : public Record {
};

class CopySolveRecord : public ConstraintSolveRecord {
};

class GepSolveRecord : public Record {
};

class MergeCycleRecord : public Record { 

};

#endif
