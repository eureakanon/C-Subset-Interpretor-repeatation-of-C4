#include<stdio.h>
#include<stdlib.h>
#include<memory.h>
#include<string.h>
#define int long long
int token;//current token
char* src,*old_src;//源代码
int poolsize;//text和stack的默认长度
int line;//行数

int* text;//code segment
int* old_text;
int* stack;//stack segment
char* data;//data segment

int* pc;//程序计数器,下一条指令的内存地址
int* sp;//栈顶指针寄存器
int* bp;//基址寄存器
int ax;//通用寄存器
int cycle;//这玩意干什么的
enum { LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT };//指令集
enum{
    Num = 128,Fun, Sys, Glo, Loc, Id,
  Char, Else, Enum, If, Int, Return, Sizeof, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};//标记

int token_id;
int *current_id;
int *symbols;
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};
int token_val;
/*next函数将字符串转化为标记流，目的就是一个
得到token=?这是something belong to 标记
得到token_val,这代表了这个标记的值，
比如在解析字符串的时候就会返回内存首地址作为token_val然后数字就会为其值作为token_val
而变量不一般:它需要注册到symbols作为新的标记存在
*/
void next()
{
    char* last_pos;
    int hash;
    while(token=*src)
    {
        ++src;
        if(token=='\n')
        {
            ++line;//遇到了换行符直接line+1
        }
        else if(token=='#')
        {
            while(*src!=0&&*src!='\n')
            {
                src++;
            }
        }/*不支持宏定义，谢谢*/
        else if((token>='a'&&token<='z')||(token>='A'&&token<='Z')||(token=='_'))//接受变量的定义
        {
            last_pos=src-1;/*因为src是下一个字符*/
            hash=token;
            while((*src>='a'&&*src<='z')||(*src>='A'&&*src<='Z')||(*src=='_')||(*src>='0'&&*src<='9'))
            {
                hash=hash*147+*src;/*经典的hash算法，防止hash冲突*/
                src++;
            }/*一直检查下一个字符直至不再是数字，字母，下划线*/
            current_id=symbols;//查找定义的符号表
            while(current_id[Token])
            {
                if(current_id[Hash]==hash&&!memcmp((char*)current_id[Name],last_pos,src-last_pos))
                {
                    token=current_id[Token];
                    return;
                }
                current_id=current_id+IdSize;
            }
            /*注册新的ID*/
            current_id[Name]=(int)last_pos;
            current_id[Hash]=hash;
            token=current_id[Token]=Id;
            return;
        }
        /*
    OK,我们不妨来解析一下这段代码的意思
    while(token=*src)这就是在说token=*src
    然后循环体的第一行++src;代表着token为当前字符,src为下一个字符
    我们只解析检测到token为数字，字母，下划线的时候
    last_pos=src-1,记录标识符开始的位置
    hash=token，哈希值被赋值为第一个字符的ascii码值
    如果下一个字符src依旧是字母，数字，下划线,hash=hash*147+*src,然后再看一个字符
    计算这个标识符的哈希值
    current_id=symbols，从symbols中找到Token位置不为零的，
    然后看看哈希值是否匹配，然后从last_pos到src-last_pos这部分找到Name是否匹配
    如果匹配就token=current_id[Token]，然后进入下一个标识符
    */
   /*
   就是线性搜索:
   在已经定义的symbol表中按token搜索,也就是已经注册的标识符中寻找
   hash相同且Name也一样的标识符
   找到了就可以return
   没找到判断src是不是还 \in {数字,字母,下划线}，并返回*位置
   */
        else if(token>='0'&&token<='9')
        {
            // parse number, three kinds: dec(123) hex(0x123) oct(017)
            token_val=token-'0';
            if(token_val>0)
            {
                while(*src>='0'&&*src<='9')
                {
                    token_val=token_val*10+*src++-'0';
                    /*您当然可以注意到
                如果我们读到了一个数字token,下一个字符依旧在0~9,则token_val=token_val*10+*src-'0'
                src++
                    */
                }
            }
            else/*hex，begin with 0*/
            {
                if(*src=='x'||*src=='X')
                {
                    token=*++src;/*希望您没有忘记这样一件事:
                    *++src <=> ++src, *src
                    换句话说，就是,检测到*src==x||*src==X
                    让token读到0x之后的那个数
                    */
                   while((token>='0'&&token<='9')||(token>='A'&&token<='F')||(token>='a'&&token<='f'))
                   {
                        token_val=token_val*16+(token & 15)+(token >= 'A' ? 9 : 0);
                        token=*++src;
                   }
               }
               else
               {
                    while(*src>='0'&&*src<='7')
                    {
                        token_val=token_val*8+*src++-'0';
                    }
               }
            }
            token=Num;
            return;
        }
        else if((token=='"')||(token=='\''))/*注意这里采用了转义字符\'代表单个'*/
        {
            last_pos=data;/*希望您没有忘记汇编语言的知识:字符串储存于数据段的缓冲区*/
            while(*src!=0&&*src!=token){
                token_val=*src++;
                if(token_val=='\\')/*遇到转义字符,需要多看一位*/
                {
                    token_val=*src++;
                    if(token_val=='n')
                    {
                        token_val = '\n';/*遇到换行符*/
                    }
                }
                if(token=='"')
                {
                    *data++=token_val;
                }
            }/*如果没有闭合或者没有终止符*/
            src++;/*注意*src==token or *src==0,所以要多看一位*/
            if(token=='"')
            {
                token_val=last_pos;
            }
            else
            {
                token=Num;
            }
            return;
        }
        else if(token=='/')
        {
            if(*src=='/')
            {
                while(*src!=0&&*src!='\n')
                {
                    src++;
                }
            }
            else
            {
                token=Div;
                return;
            }
        }
        else if(token=='=')
        {
            if(*src=='=')
            {
                src++;
                token=Eq;
            }
            else
            {
                token=Assign;
            }
            return;
        }
        else if(token=='+')
        {
            if(*src=='+')
            {
                src++;
                token=Inc;
            }
            else
            {
                token=Add;
            }
            return;
        }
        else if(token=='-')
        {
            if(*src=='-')
            {
                src++;
                token=Dec;
            }
            else
            {
                token=Sub;
            }
            return;
        }
        else if(token=='!')
        {
            if(*src=='=')
            {
                src++;
                token=Ne;
            }
            return;
        }
        else if(token=='<')
        {
            if(*src=='=')
            {
                src++;
                token=Le;
            }
            else if(*src=='<')
            {
                src++;
                token=Shl;
            }
            else
            {
                src++;
                token=Lt;
            }
            return;
        }
        else if (token == '>') {
            // parse '>=', '>>' or '>'
            if (*src == '=') {
                src ++;
                token = Ge;
            } else if (*src == '>') {
                src ++;
                token = Shr;
            } else {
                token = Gt;
            }
            return;
        }
        else if(token=='|')
        {
            if(*src=='|')
            {
                src++;
                token=Lor;
            }
            else
            {
                token=Or;
            }
            return;
        }
        else if(token=='&')
        {
            if(*src=='&')
            {
                src++;
                token=Lan;
            }
            else
            {
                token=And;
            }
            return;
        }
        else if (token == '^') {
            token = Xor;
            return;
        }
        else if (token == '%') {
            token = Mod;
            return;
        }
        else if (token == '*') {
            token = Mul;
            return;
        }
        else if (token == '[') {
            token = Brak;
            return;
        }
        else if (token == '?') {
            token = Cond;
            return;
        }
         else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') {
            // directly return the character as token;
            return;
        }

    }
    return;
}
void match(int tk)/*封装next函数用于匹配*/
{
    if(token==tk)
    {
        next();
    }
    else
    {
        printf("%d,unexpected token:%d\n",line,tk);
        exit(-1);
    }
}
enum{
    CHAR,INT,PTR
};
int basetype;/*变量定义的基本类型*/
int expr_type;
int index_of_bp;
void expression(int level) {
    int* id;
    int tmp;
    int* addr;
    if (!token) {
            printf("%d: unexpected token EOF of expression\n", line);
            exit(-1);
    }
    if(token==Num)
    {
        match(Num);
        /*解析到数字*/
        *++text=IMM;
        *++text=token_val;
        expr_type = INT;
    }
    else if(token=='"')
    {
        *++text=IMM;
        *++text=token_val;
        match('"');
        while(token=='"')
        {
            match('"');
        }
        data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));
        expr_type = PTR;
    }
    else if(token==Sizeof)
    {
        /*解析表达式:sizeof(int or char or ptr)*/
        match(Sizeof);
        match('(');
        expr_type=INT;/*注意这里的expr_type指的是Sizeof里面的式子的性质，用于IMM指令后面的立即数的表示*/
        if(token==Int)
        {
            match(Int);
            expr_type=INT;
        }
        else if(token==Char)
        {
            match(Char);
            expr_type=CHAR;
        }
        while(token==Mul)
        {
            match(Mul);
            expr_type=expr_type+PTR;
        }
        match(')');
        *++text=IMM;
        *++text=(expr_type==Char)?sizeof(char):sizeof(int);

        expr_type=INT;
    }
    else if(token==Id)
    {
        match(Id);
        id=current_id;
        if(token=='(')
        {
            match('(');
            tmp=0;
            while(token!=')')
            {
                expression(Assign);
                *++text=PUSH;
                tmp++;
                
                if(token==',')
                {
                    match(',');
                }
            }
            match(')');
            /*以上是函数的调用，为函数的参数分配了内存*/
            if(id[Class]==Sys)
            {
                /*系统函数的调用*/
                *++text=id[Value];
            }
            else if(id[Class]==Fun)
            {
                *++text=CALL;
                *++text=id[Value];/*在global_declaration的时候会为函数声明一个调用地址*/
            }
            else/*无法解析到函数*/
            {
                printf("%d,bad function call\n",line);
                exit(-1);
            }
            if(tmp>0)
            {
                *++text=ADJ;
                *++text=tmp;
            }/*参数变量有占用内存,释放*/
            expr_type=id[Type];
        }
        else if(token==Num)
        {
            *++text=IMM;
            *++text=id[Value];
        }/*阅读到了数字常量*/
        else
        {
            if(id[Class]==Loc)
            {
                *++text=LEA;
                *++text=index_of_bp-id[Value];
            }
            else if(id[Class]==Glo)
            {
                *++text=IMM;
                *++text=id[Value];
            }
            else
            {
                printf("%d,undefined variable\n",line);
            }
            expr_type = id[Type];
            *++text = (expr_type == Char) ? LC : LI;
        }/*解析局部的和全局的变量*/
    }
    else if(token=='(')
    {
        match('(');
        if(token==Char||token==Int)
        {
            tmp=(token==Char)?CHAR:INT;
            match(token);
            while(token==Mul)
            {
                match(Mul);
                tmp=tmp+PTR;
            }
            match(')');
            expression(Inc); // cast has precedence as Inc(++)/*未来要处理的优先级问题*/
            expr_type=tmp;
        }
        else {
        // normal parenthesis
        expression(Assign);
        match(')');/*一般的括号，不是(INT) or (Char)*/
        }
    }/*强制变换*/
    else if(token==Mul)
    {
        match(Mul);
        expression(Inc);/*依旧优先级问题*/
        if(expr_type>=PTR)
        {
            expr_type=expr_type-PTR;
        }
        else
        {
            printf("%d, bad derefrences\n",line);
            exit(-1);
        }
        *++text = (expr_type == CHAR) ? LC : LI;/*如果还是char*，就将地址作为数字存储在ax中*/
    }/*处理指针*/
    else if(token==And)
    {
        match(And);
        expression(Inc);
        if(*text==LC||*text==LI)
        {
            text--;
        }
        else
        {
            printf("%d,bad address\n",line);
            exit(-1);
        }
        expr_type = expr_type + PTR;
        /*
        int* b=&a; <=>
        IMM <addr>
        LC or LI
        我们不执行LC,就是IMM <addr>,ax=addr=&a
        */
    }
    else if(token=='!')
    {
        match('!');
        expression(Inc);

        *++text=PUSH;
        *++text=IMM;
        *++text=0;
        *++text=EQ;
        expr_type=INT;
    }/*逻辑取反
    栈顶为0
    ax与0相比较
    ax==1=>ax=0
    ax==0=>ax=1
    */
   else if(token=='~')
   {
       match('~');
       expression(Inc);

       *++text=PUSH;
       *++text=IMM;
       *++text=-1;
       *++text=XOR;
       expr_type=INT;
   }
   else if(token==Add)
   {
        match(Add);
        expression(Inc);

        expr_type=INT;
   }
   else if(token==Sub)
   {
        match(Sub);
        if(token==Num)
        {
            *++text=IMM;
            *++text=-token_val;
            match(Num);
        }
        else
        {
            *++text=IMM;
            *++text=-1;
            *++text=PUSH;
            expression(Inc);
            *++text=MUL;
        }/*这个分支可能是Id,比如-a,于是我们要先IMM -1,之后再递归解析expression(Inc),随后MUL*/
        expr_type=INT;
   }
   else if(token==Inc||token==Dec)
   {
        tmp=token;
        match(token);
        expression(Inc);
        if(*text==LC)
        {
            *text=PUSH;
            *++text=LC;
        }
        else if(*text==LI)
        {
            *text=PUSH;
            *++text=LI;
        }
        else
        {
            printf("%d,bad left value pre-increment\n",line);
        }
        *++text=PUSH;
        *++text=IMM;

        *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
        *++text = (tmp == Inc) ? ADD : SUB;
        *++text = (expr_type == CHAR) ? SC : SI;
   }
   else{
        printf("%d, bad expression\n",line);
   }
       {
        while (token >= level) {
            // handle according to current operator's precedence
            tmp = expr_type;
            if (token == Assign) {
                // var = expr;
                match(Assign);
                if (*text == LC || *text == LI) {
                    *text = PUSH; // save the lvalue's pointer
                } else {
                    printf("%d: bad lvalue in assignment\n", line);
                    exit(-1);
                }
                expression(Assign);

                expr_type = tmp;
                *++text = (expr_type == CHAR) ? SC : SI;
            }
            else if (token == Cond) {
                // expr ? a : b;
                match(Cond);
                *++text = JZ;
                addr = ++text;
                expression(Assign);
                if (token == ':') {
                    match(':');
                } else {
                    printf("%d: missing colon in conditional\n", line);
                    exit(-1);
                }
                *addr = (int)(text + 3);
                *++text = JMP;
                addr = ++text;
                expression(Cond);
                *addr = (int)(text + 1);
            }
            else if (token == Lor) {
                // logic or
                match(Lor);
                *++text = JNZ;
                addr = ++text;
                expression(Lan);
                *addr = (int)(text + 1);
                expr_type = INT;
            }
            else if (token == Lan) {
                // logic and
                match(Lan);
                *++text = JZ;
                addr = ++text;
                expression(Or);
                *addr = (int)(text + 1);
                expr_type = INT;
            }
            else if (token == Or) {
                // bitwise or
                match(Or);
                *++text = PUSH;
                expression(Xor);
                *++text = OR;
                expr_type = INT;
            }
            else if (token == Xor) {
                // bitwise xor
                match(Xor);
                *++text = PUSH;
                expression(And);
                *++text = XOR;
                expr_type = INT;
            }
            else if (token == And) {
                // bitwise and
                match(And);
                *++text = PUSH;
                expression(Eq);
                *++text = AND;
                expr_type = INT;
            }
            else if (token == Eq) {
                // equal ==
                match(Eq);
                *++text = PUSH;
                expression(Ne);
                *++text = EQ;
                expr_type = INT;
            }
            else if (token == Ne) {
                // not equal !=
                match(Ne);
                *++text = PUSH;
                expression(Lt);
                *++text = NE;
                expr_type = INT;
            }
            else if (token == Lt) {
                // less than
                match(Lt);
                *++text = PUSH;
                expression(Shl);
                *++text = LT;
                expr_type = INT;
            }
            else if (token == Gt) {
                // greater than
                match(Gt);
                *++text = PUSH;
                expression(Shl);
                *++text = GT;
                expr_type = INT;
            }
            else if (token == Le) {
                // less than or equal to
                match(Le);
                *++text = PUSH;
                expression(Shl);
                *++text = LE;
                expr_type = INT;
            }
            else if (token == Ge) {
                // greater than or equal to
                match(Ge);
                *++text = PUSH;
                expression(Shl);
                *++text = GE;
                expr_type = INT;
            }
            else if (token == Shl) {
                // shift left
                match(Shl);
                *++text = PUSH;
                expression(Add);
                *++text = SHL;
                expr_type = INT;
            }
            else if (token == Shr) {
                // shift right
                match(Shr);
                *++text = PUSH;
                expression(Add);
                *++text = SHR;
                expr_type = INT;
            }
            else if (token == Add) {
                // add
                match(Add);
                *++text = PUSH;
                expression(Mul);

                expr_type = tmp;
                if (expr_type > PTR) {
                    // pointer type, and not `char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                *++text = ADD;
            }
            else if (token == Sub) {
                // sub
                match(Sub);
                *++text = PUSH;
                expression(Mul);
                if (tmp > PTR && tmp == expr_type) {
                    // pointer subtraction
                    *++text = SUB;
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = DIV;
                    expr_type = INT;
                } else if (tmp > PTR) {
                    // pointer movement
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                    *++text = SUB;
                    expr_type = tmp;
                } else {
                    // numeral subtraction
                    *++text = SUB;
                    expr_type = tmp;
                }
            }
            else if (token == Mul) {
                // multiply
                match(Mul);
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;
                expr_type = tmp;
            }
            else if (token == Div) {
                // divide
                match(Div);
                *++text = PUSH;
                expression(Inc);
                *++text = DIV;
                expr_type = tmp;
            }
            else if (token == Mod) {
                // Modulo
                match(Mod);
                *++text = PUSH;
                expression(Inc);
                *++text = MOD;
                expr_type = tmp;
            }
            else if (token == Inc || token == Dec) {
                // postfix inc(++) and dec(--)
                // we will increase the value to the variable and decrease it
                // on `ax` to get its original value.
                if (*text == LI) {
                    *text = PUSH;
                    *++text = LI;
                }
                else if (*text == LC) {
                    *text = PUSH;
                    *++text = LC;
                }
                else {
                    printf("%d: bad value in increment\n", line);
                    exit(-1);
                }

                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? ADD : SUB;
                *++text = (expr_type == CHAR) ? SC : SI;
                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? SUB : ADD;
                match(token);
            }
            else if (token == Brak) {
                // array access var[xx]
                match(Brak);
                *++text = PUSH;
                expression(Assign);
                match(']');

                if (tmp > PTR) {
                    // pointer, `not char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                else if (tmp < PTR) {
                    printf("%d: pointer type expected\n", line);
                    exit(-1);
                }
                expr_type = tmp - PTR;
                *++text = ADD;
                *++text = (expr_type == CHAR) ? LC : LI;
            }
            else {
                printf("%d: compiler error, token = %d\n", line, token);
                exit(-1);
            }
        }
    }
}//解析表达式
void statement()
{
    int* a;
    int* b;
    if(token==If)
    {
        match(If);
        match('(');
        expression(Assign);
        match(')');
        *++text=JZ;
        b=++text;

        statement();/*递归防止If(While)*/
        if(token==Else)
        {
            match(Else);
            *b=(int)(text+3);
            *++text=JMP;
            b=++text;

            statement();
        }
        *b = (int)(text + 1);
    }
    /*
    condition
    JZ text+3(Else分句)
    JMP text+1(If-Else结束之后要执行的分句)
    */
    else if(token==While)
    {
        match(While);
        a=text+1;
        match('(');
        expression(Assign);
        match(')');
        *++text=JZ;
        b=++text;
        statement();
        *++text=JMP;
        *++text=(int)a;
        *b=(int)(text+1);
    }
    /*
    这一段的确我看懂了
    那就是:
    a=text+1
    condition
    JZ Out-of-loop
    JMP a
    */
    else if (token == Return) {
    // return [expression];
    match(Return);

    if (token != ';') {
        expression(Assign);
    }

    match(';');

    // emit code for return
    *++text = LEV;
    }
    else if(token=='{')
    {
        match('{');
        while(token!='}')
        {
            statement();
        }
        match('}');
    }
    else if (token == ';') {
    // empty statement
    match(';');
    }
    else {
    // a = b; or function_call();
    expression(Assign);
    match(';');
    }
}
void enum_declaration()
{
    int i;
    i=0;
    while(token!='}')
    {
        if(token!=Id)
        {
            printf("%d,bad enum identifier,%d\n",line,token);/*所有的enum需要满足enum[id]{id=1,id=2,...};*/
            exit(-1);
        }
        next();
        if(token==Assign)
        {
            next();
            if(token!=Num)/*在next部分我们alternate token_val or token or注册了新的变量,如果最后不是Num就报错*/
            {
                printf("%d,bad enum initializer,%d\n",line,token);
            }
            i=token_val;
            next();
        }
        current_id[Class] = Num;
        current_id[Type] = INT;
        current_id[Value] = i++;/*这个设计是很厉害的，因为它保证了下一个没有定显式定义的变量自增*/
        if(token==',')
        {
            next();
        }
    }
}
void function_parameter()
{
    int type;
    int params;
    params=0;/*参数数量为0*/
    while(token!=')')/*还未闭合*/
    {
        type=INT;/*基础类型*/
        if(token==Int)
        {
            match(Int);
        }
        else if(token==Char)
        {
            match(Char);
            type=CHAR;
        }
        while(token==Mul)
        {
            match(Mul);
            type=type+PTR;
        }
        if(token!=Id)
        {
            printf("%d,bad parameter declaration \n",line);
        }
        if(current_id[Class]==Loc)/*与局部变量相同就报错，与全局变量相同就覆盖，逻辑是如下的*/
        {
            printf("%d,duplicate parameter declaration \n",line);
        }
        match(Id);
        current_id[BClass]=current_id[Class];current_id[Class]=Loc;
        current_id[BType]=current_id[Type];current_id[Type]=type;
        current_id[BValue] = current_id[Value]; current_id[Value]  = params++;
        if(token==',')
        {
            match(',');
        }
    }
    index_of_bp=params+1;
}
void function_body()
{
    int pos_local;
    int type;
    pos_local=index_of_bp;
    while(token==Int||token==Char)
    {
        basetype=(token==Int)?INT:CHAR;
        match(token);
        while(token!=';')
        {
            type=basetype;
            while(token==Mul)
            {
                match(Mul);
                type=type+PTR;
            }
            if(token!=Id)
            {
                printf("%d,bad declaration\n",line);
                exit(-1);
            }
            if(current_id[Class]==Loc)
            {
                printf("%d,duplicate local declaration\n",line);
                exit(-1);
            }
            match(Id);
            current_id[BClass]=current_id[Class];current_id[Class]=Loc;
            current_id[BType]=current_id[Type];current_id[Type]=type;
            current_id[BValue]=current_id[Value];current_id[Value]=++pos_local;
            if(token==',')
            {
                match(',');
            }
        }
        match(';');
    }
    *++text=ENT;/*这里的text代表代码段*/
    *++text=pos_local-index_of_bp;
    while(token!='}')
    {
        statement();
    }
}
void function_declaration()
{
    match('(');
    function_parameter();
    match(')');
    match('{');
    function_body();
    current_id=symbols;
    while(current_id[Token])
    {
       if(current_id[Class]==Loc)/*如果是局部变量*/
       {
            current_id[Class]=current_id[BClass];
            current_id[Type]=current_id[BType];
            current_id[Value]=current_id[BValue];
       }
       current_id=current_id+IdSize;
    }
}
void global_declaration()
{
    int type;
    int i;

    basetype=INT;/*默认类型是INT*/
    if(token==Enum)
    {
        match(Enum);/*当前已经消耗Enum这个token转化为下一个token*/
        if(token!='{')/*如果不是左花括号，就识别Id*/
        {
            match(Id);/*检查当前的token是不是Id类型*/
        }
        if(token=='{')
        {
            match('{');
            enum_declaration();
            match('}');
        }
        match(';');
        return;
    }
    if(token==Int)
    {
        match(Int);
    }
    else if(token==Char)
    {
        match(Char);
        basetype=CHAR;
    }
    while(token!=';'&&token!='}')
    {
        type=basetype;
        /*我们要处理指针类型*/
        while(token==Mul)
        {
            match(Mul);
            type=type+PTR;/*这里标识指针类型为type+PTR*/
        }
        if(token!=Id)
        {
            printf("%d,bad global declaration\n",line);/*int ****的尽头不是变量*/
            exit(-1);
        }
        if(current_id[Class])/*发现已经存在了这个全局定义*/
        {
            printf("%d,duplicate global declaration\n",line);
            exit(-1);
        }
        match(Id);
        current_id[Type]=type;
        if(token=='(')
        {
            current_id[Class]=Fun;
            current_id[Value]=(int)(text+1);/*text是code segment，然后下一条要执行的指令是这个function的地址*/
            function_declaration();
        }
        else/*不是函数而是全局变量*/
        {
            current_id[Class]=Glo;/*代表它是全局变量*/
            current_id[Value]=(int)(data);
            data=data+sizeof(int);
        }
        if(token==',')
        {
            match(',');
        }

    }
    next();

}

void program()
{
    next();
    while(token>0)
    {
        global_declaration();
    }
    return;
}//语法分析的入口
int eval()//虚拟机的入口,解释目标代码
{
    int op,*tmp;//op为operation
    while(1)
    {
        op=*pc++;
        if(op==IMM){ax=*pc++;}//pc是下一条指令,IMM <num>的下一条指令显然是<num>,IMM=> ax=*pc,pc++
        else if(op==LC){ax=*(char*)(ax);}//LC==Load Char,把ax作为内存地址中存储的内容以字符类型解读，保存回到ax
        else if(op==LI){ax=*(int*)(ax);}//LI=Load Integer
        else if(op==SC){ax=*(char*)*sp++=ax;}//Save Char,ax写入栈顶,随后栈顶弹出，最后ax为弹出的那个值
        else if(op==SI){*(int*)*sp++=ax;}//Load Int,ax写入栈顶,随后栈顶弹出来
        else if(op==PUSH){*--sp=ax;}//入栈,sp=ax,sp--
        else if(op==JMP){pc=(int*)*pc;}//JMP addr,此时pc指向addr,然后pc解应用得到addr的值,随后又按照addr作为整数指针来操作
        else if(op==JZ){pc=ax?pc+1:(int*)*pc;}//ax=0,JMP,ax!=0 pc++
        else if(op==JNZ){pc=ax?(int*)*pc:pc+1;}//reverse
        else if(op==CALL){*--sp=(int)(pc+1);pc=(int*)*pc;}//pc+1代表返回地址，，就是将返回地址入栈，随后跳转到调用的那个地址
        else if(op==ENT){*--sp=(int)bp;
            bp=sp;
            sp=sp-*pc++;
        }
        /*
        这个指令可了不得了,指令的原型是ENT <NUM>代表为函数中的局部变量分配栈内存
        第一行:bp是基址,它作为整数的时候会返回0x07891之类的数字，它入栈，类似与pushq %rbp,只不过我们限制为int类型
        第二行:bp=sp，基址为新栈顶的位置，未来会分配栈内存
        第三行:sp=sp-*pc++,假如有局部变量i,*pc=3,则sp为i分配三个内存,pc++
        */
       else if(op==ADJ){sp=sp+*pc++;}//调整栈指针
       else if(op==LEV){sp=bp;
        bp=(int*)*sp++;
        pc=(int*)*sp++;
        }
        /*
        标号高地址
    +---------------------+
    | 调用者的其他数据     |
    +---------------------+
    | 参数（第N个...第1个）|   ← 调用者压入
    +---------------------+
    | 返回地址（CALL压入）  |
    +---------------------+
    | 旧的bp（ENT压入）     |   ← bp 指向这里
    +---------------------+
    | 局部变量 1           |
    | ...                 |
    | 局部变量 N           |   ← sp 指向这里（低地址）
    +---------------------+
        标号低地址
        这个指令也是不得了的
        sp=bp,释放所有的局部变量
        bp=(int*)*sp++,bp恢复,sp++
        pc=(int*)*sp++,pc回到返回地址，sp++
        */
       else if(op==LEA){ax=(int)(bp+*pc++);}/*如上图,bp以上的内存得到参数,LEA <num>,pc指向num,在解应用得到<num>之后bp=bp+num就得到了指定的参数
       */
        /*以下是运算符指令*/
        else if (op == OR)  ax = *sp++ | ax;
        else if (op == XOR) ax = *sp++ ^ ax;
        else if (op == AND) ax = *sp++ & ax;
        else if (op == EQ)  ax = *sp++ == ax;
        else if (op == NE)  ax = *sp++ != ax;
        else if (op == LT)  ax = *sp++ < ax;
        else if (op == LE)  ax = *sp++ <= ax;
        else if (op == GT)  ax = *sp++ >  ax;
        else if (op == GE)  ax = *sp++ >= ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
        else if (op == ADD) ax = *sp++ + ax;
        else if (op == SUB) ax = *sp++ - ax;
        else if (op == MUL) ax = *sp++ * ax;
        else if (op == DIV) ax = *sp++ / ax;
        else if (op == MOD) ax = *sp++ % ax;
        /*
        这些都一样啊，ax operation *sp,sp++
        */
       else if(op==EXIT){printf("exit(%d)\n",*sp);
        return *sp;
        }
        /*
        OPERATION==EXIT，输出 exit *sp,返回*sp的内容
        */
       else if(op==OPEN){ax=open((char*)sp[1],sp[0]);}
       else if(op==CLOS){close(*sp);}
       else if(op==READ){ax=read(sp[2],(char*)sp[1],*sp);}
       else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
       else if (op==MALC){ax=(int)malloc(*sp);}
       else if(op==MSET){ax=(int)memset((char *)sp[2], sp[1], *sp);}
       else if (op==MCMP){ax=(int)memcmp((char *)sp[2], (char *)sp[1], *sp);}
       /*
            这里的op基本上是调用系统函数啦,
            但是注意所有的参数都是栈上，
            参数N在sp[0]
            而参数0反而在sp[N]
            因为入栈是--sp
       */
       else
       {
            printf("Unknown instruction:%d\n",op);
            return -1;
       }
    }
    return 0;
}
int *idmain;//主函数
int main(int argc, char **argv)
{
        int i, fd;
    int *tmp;

    argc--;
    argv++;

    poolsize = 256 * 1024; // arbitrary size
    line = 1;

    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }

    // allocate memory for virtual machine
    if (!(text = old_text = malloc(poolsize))) {
        printf("could not malloc(%d) for text area\n", poolsize);
        return -1;
    }
    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }
    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }
    if (!(symbols = malloc(poolsize))) {
        printf("could not malloc(%d) for symbol table\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);
    memset(symbols, 0, poolsize);
    bp = sp = (int *)((int)stack + poolsize);
    ax = 0;

    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void main";

     // add keywords to symbol table
    i = Char;
    while (i <= While) {
        next();
        current_id[Token] = i++;
    }

    // add library to symbol table
    i = OPEN;
    while (i <= EXIT) {
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;
        current_id[Value] = i++;
    }

    next(); current_id[Token] = Char; // handle void type
    next(); idmain = current_id; // keep track of main


    // read the source file
    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }

    if (!(src = old_src = malloc(poolsize))) {
        printf("could not malloc(%d) for source area\n", poolsize);
        return -1;
    }
    // read the source file
    if ((i = read(fd, src, poolsize-1)) <= 0) {
        printf("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0; // add EOF character
    close(fd);

    program();

    if (!(pc = (int *)idmain[Value])) {
        printf("main() not defined\n");
        return -1;
    }

    // setup stack
    sp = (int *)((int)stack + poolsize);
    *--sp = EXIT; // call exit if main returns
    *--sp = PUSH; tmp = sp;
    *--sp = argc;
    *--sp = (int)argv;
    *--sp = (int)tmp;

    return eval();
}
