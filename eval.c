KatieVal *reduce_val(KatieEnv *env, KatieVal *val);

KatieVal *eval(KatieEnv *env, KatieVal *val) {
    KatieVal *newList, *first;

    if (val->kind != KatieValKind_List) {
        return reduce_val(env, val);
    }

    if (array_is_empty(val->as.list)) return NULL;

    newList = reduce_val(env, val);
    if (!newList) return NULL;
    if (array_is_empty(newList->as.list) || array_length(newList->as.list) < 2) return NULL;

    first = newList->as.list[0];
    if (first->kind != KatieValKind_Proc) {
        Unreachable();
    }

    return first->as.proc(env, array_length(newList->as.list) - 1, &newList->as.list[1]);
}

KatieVal *reduce_val(KatieEnv *env, KatieVal *val) {
    switch (val->kind) {
    case KatieValKind_Number:
    case KatieValKind_Bool: return val;

    case KatieValKind_Symbol: {
        KatieVal *newVal = env_lookup(env, val->as.symbol);
        if (!newVal) {
            Unreachable();
        }
        return newVal;
    };

    case KatieValKind_List: {
        Array(KatieVal *) newList;
        init_array(newList);
        array_for_each(val->as.list, i) { array_push(newList, eval(env, val->as.list[i])); }
        return alloc_list(newList);
    } break;

    default: Unreachable();
    }
}
