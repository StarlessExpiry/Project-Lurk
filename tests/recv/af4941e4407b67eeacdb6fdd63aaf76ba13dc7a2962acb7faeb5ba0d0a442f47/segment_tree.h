#define MAXDATAV 100

typedef double seg_tree[100];

struct st_pos {
    int root;
    int left;
    int right;
};

// 作用:    传入将要被构造为线段树st的数组
// 参数:    struct st_pos * start_position  构建的线段树从何处创建（以谁为根）,
//                                          其中root应固定为0, left和right为待构造区间;
//          int * a                         create函数依据a数组来构建线段树;
//          char * val_flags                线段树的val值应按照什么形式由下到上进行设置, 例如"sum, max, min" 
// 返回值:  int                             成功创建时返回0, 错误时返回-1;
// 例子:    int a[8] = {2, 5, 7, 3, 6, 9, 4, 10};
//          struct st_pos start_position = {0, 0, 7};
//          create(&start_position, a);
int
create(seg_tree st, struct st_pos * const start_position, int * const a, const char * val_flags);

int
query_one(seg_tree st, struct st_pos * const start_position, int * const a);

int
query_range(seg_tree st, struct st_pos * const start_position, int * const a);
