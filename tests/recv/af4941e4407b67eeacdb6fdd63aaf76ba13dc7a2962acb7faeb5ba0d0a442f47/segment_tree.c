#include "segment_tree.h"
#include "tools/msg_print_algo.h"
#include <math.h>
#include <string.h>



int
create(seg_tree st, struct st_pos *const start_position, int *const a, const char * val_flags)
{
    int root = start_position->root;
    int left= start_position->left;
    int right = start_position->right;
    

    int rtn = 0;;
    // 递归终止条件, 在left和right相等时, 构造线段树叶子节点, 例如a[1:1]
    if (left == right) {
        st[root] = a[left];
#ifdef DEBUG
        msg_dbg_f("segment_tree/create: st[%d : %d] = a[%2d]", root, root, left);
#endif
    }
    else {
        // 折半构建的方式
        int mid = (left + right) / 2;
        struct st_pos left_pos = {2 * root + 1, left, mid};
        struct st_pos right_pos = {2 * root + 2, mid+1, right};

        // 递归创建左子树, 递归创建右子树
#ifdef DEBUG
        msg_dbg_f("segment_tree/create: 创建left区间[%d : %d]", left_pos, right_pos);
#endif
        if (create(st, &left_pos, a, val_flags) == -1)
            rtn |= -1;
#ifdef DEBUG
        msg_dbg_f("segment_tree/create: 创建right区间[%d : %d]", left_pos, right_pos);
#endif
        if (create(st, &right_pos, a, val_flags) == -1)
            rtn |= -1;

        // 向上传递
        if (strncmp(val_flags, "sum", 3) == 0)
            st[root] = st[2*root+1] + st[2*root+2];
        else if (strncmp(val_flags, "min", 3) == 0)
            st[root] = fmin(st[2*root+1], st[2*root+1]);
        else if (strncmp(val_flags, "max", 3) == 0)
            st[root] = fmax(st[2*root+1], st[2*root+2]);
    }
    return rtn;
}
