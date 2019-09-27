#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct d
{ //data 저장 struct
    int data;
    int addr; //상대주소로 표현 16자리
    int arr;
    char name[50];
} dataarr[500];

struct t
{                          //text 저장 struct
    int address;           //상대주소
    int mode;              //명령어의 특징에 맞게 모드 부여
    char op[7];            //6개씀
    char s[6], t[6], d[6]; // d=s (op) t
    char shamt[7];         //6개씀
    char func[7];          //6개씀
    char addr[27];         //26개 씀
    char imm[17];          //16개 씀
} textarr[500];

struct l
{ //loop위치 저장 struct
    char name[50];
    int addr;
} loop[5000];

const struct in
{                   //명령어 세트
    char oplit[10]; //문자
    char op[7];     //hex
    char funct[7];  //hex
    char mode;      //r,i,j
} instruction[21] = {
    //opcode,16진수 앞, 뒤, 형식
    //{d,s,t} == 0,      {t s imm} == 1,  {t s addr} == 2
    //{addr} == 3,       {s} == 4,        {t imm} == 5
    //{d t shamt} == 6,  {la} == 7        lw는 특별해서 따로 ==8
    {"addiu", "001001", "", '1'},      //i  t s imm
    {"addu", "000000", "100001", '0'}, //r  d s t
    {"and", "000000", "100100", '0'},  //r  d s t
    {"andi", "001100", "", '1'},       //i  t s imm
    {"beq", "000100", "", '2'},        //i  t s addr
    {"bne", "000101", "", '2'},        //i  t s addr
    {"j", "000010", "", '3'},          //j  addr(26)
    {"jal", "000011", "", '3'},        //j  addr(26)
    {"jr", "000000", "001000", '4'},   //r  s
    {"lui", "001111", "", '5'},        //i  t imm
    {"lw", "100011", "", '8'},         //i  t s imm 이지만 특별
    {"la", "", "", '7'},               //   안에서 lui ori돌리자.
    {"nor", "000000", "100111", '0'},  //r  d s t
    {"or", "000000", "100101", '0'},   //r  d s t
    {"ori", "001101", "", '1'},        //i  t s imm
    {"sltiu", "001011", "", '1'},      //i  t s imm
    {"sltu", "000000", "101011", '0'}, //r  d s t
    {"sll", "000000", "000000", '6'},  //r  d t shamt
    {"srl", "000000", "000010", '6'},  //r  d t shamt
    {"sw", "101011", "", '8'},         //i  t s imm 이지만 특별
    {"subu", "000000", "100011", '0'}  //r  d s t
};

/*******************************************************
 * Function Declaration
 *
 *******************************************************/
char *change_file_ext(char *str);
char *itob(int k, int j);
char **strtokenize(char s[]);

/*******************************************************
 * Function: main
 *
 * Parameters:
 *  int
 *      argc: the number of argument
 *  char*
 *      argv[]: array of a sting argument
 *
 * Return:
 *  return success exit value
 *
 * Info:
 *  The typical main function in C language.
 *  It reads system arguments from terminal (or commands)
 *  and parse an assembly file(*.s).
 *  Then, it converts a certain instruction into
 *  object code which is basically binary code.
 *
 *******************************************************/
int main(int argc, char *argv[])
{
    FILE *input, *output;
    char *filename;
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <*.s>\n", argv[0]);
        fprintf(stderr, "Example: %s sample_input/example?.s\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    input = fopen(argv[1], "r");
    if (input == NULL)
    {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }
    filename = strdup(argv[1]); // strdup() is not a standard C library but fairy used a lot.
    if (change_file_ext(filename) == NULL)
    {
        fprintf(stderr, "'%s' file is not an assembly file.\n", filename);
        exit(EXIT_FAILURE);
    }
    output = fopen(filename, "w");
    if (output == NULL)
    {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    int mode, datalen = -1, textlen = -1, dataarray = -1;
    int arraycur = 0, loopcur = 0;
    char line[256];
    int tok = 0;

    while (!feof(input))
    {                                     //전체를 돌면서 main:, lab1: 등의 위치 파악
                                          //나중에 la로 인해 addr바뀌는건 뒤에 따로 구현되어있음.
        fgets(line, sizeof(line), input); //한줄 전부 입력
        char **tokened = strtokenize(line);
        if (!tok)
            if (strcmp(tokened[0], "main"))
                continue;
        tok = 1;
        textlen++;
        if (tokened[1] == NULL)
        {
            strcpy(loop[loopcur].name, tokened[0]);
            loop[loopcur++].addr = textlen--;
        }
    }

    fseek(input, 0L, SEEK_SET); //파일의 첫번째 줄로 이동
    textlen = -1;
    while (!feof(input))
    {
        fgets(line, sizeof(line), input);   //한줄 전부 입력
        char **tokened = strtokenize(line); //토큰
        if (strcmp(tokened[0], ".data") == 0)
        {
            mode = 1; //data 저장
            continue;
        }
        else if (strcmp(tokened[0], ".text") == 0)
        {
            mode = 2; //text 저장
            continue;
        }
        if (mode == 1)
        {
            datalen++; //데이터 크기

            if (strcmp(tokened[0], ".word") == 0 && !strstr(tokened[1], "0x")) //처음 문자열 = .data & 두번째 문자열에 0x 안들어있으면
                dataarr[datalen].data = atoi(tokened[1]);
            else if (strcmp(tokened[0], ".word") == 0 && strstr(tokened[1], "0x"))
                dataarr[datalen].data = strtol(tokened[1], NULL, 16);
            else if (strcmp(tokened[1], ".word") == 0 && !strstr(tokened[2], "0x")) //처음 문자열 = .data & 두번째 문자열에 0x 안들어있으면
            {
                dataarr[datalen].data = atoi(tokened[2]);
                strcpy(dataarr[datalen].name, tokened[0]);
                dataarray++;
            }
            else if (strcmp(tokened[1], ".word") == 0 && strstr(tokened[2], "0x"))
            {
                dataarr[datalen].data = strtol(tokened[2], NULL, 16);
                strcpy(dataarr[datalen].name, tokened[0]);
                dataarray++;
            }
            dataarr[datalen].addr = datalen;
            dataarr[datalen].arr = dataarray;
        }
        else if (mode == 2)
        {
            //{d,s,t} == 0,      {t s imm} == 1,  {t s addr} == 2
            //{addr} == 3,       {s} == 4,        {t imm} == 5
            //{d t shamt} == 6,  {la} == 7
            if (tokened[1] == NULL)
                continue; //loop에 대해 이미 처리되어서 무시
            textlen++;
            int i, l, d, flag = 0;
            //몇번째 인스트럭션을 쓸 건지 선택
            for (i = 0; i < 21; i++)
                if (!strcmp(tokened[0], instruction[i].oplit))
                    break;
            textarr[textlen].mode = instruction[i].mode - '0';
            strcpy(textarr[textlen].op, instruction[i].op);
            strcpy(textarr[textlen].s, "00000"); //자주 반복되는건 미리 저장.
            strcpy(textarr[textlen].d, "00000"); //나중에 덮여쓰여도 상관 없음.
            strcpy(textarr[textlen].t, "00000");
            strcpy(textarr[textlen].shamt, "00000");

            switch (textarr[textlen].mode)
            {
            case 0: //d, s, t
                strcpy(textarr[textlen].d, itob(atoi(tokened[1]), 5));
                strcpy(textarr[textlen].s, itob(atoi(tokened[2]), 5));
                strcpy(textarr[textlen].t, itob(atoi(tokened[3]), 5));
                strcpy(textarr[textlen].func, instruction[i].funct);
                break;

            case 1: //s, t, imm   addiu
                strcpy(textarr[textlen].t, itob(atoi(tokened[1]), 5));
                strcpy(textarr[textlen].s, itob(atoi(tokened[2]), 5));
                if (tokened[3][1] == 'x')
                    strcpy(textarr[textlen].imm, itob(strtol(tokened[3], NULL, 16), 16));
                else
                    strcpy(textarr[textlen].imm, itob(atoi(tokened[3]), 16));
                break;

            case 2: //s, t, addr      bne, beq
                strcpy(textarr[textlen].s, itob(atoi(tokened[1]), 5));
                strcpy(textarr[textlen].t, itob(atoi(tokened[2]), 5));
                for (l = 1; l <= loopcur; l++)
                    if (!strcmp(loop[l].name, tokened[3]))
                        break;
                strcpy(textarr[textlen].imm, itob(loop[l].addr - textlen - 1, 16));
                break;

            case 3: //addr   jump
                for (l = 1; l <= loopcur; l++)
                    if (!strcmp(loop[l].name, tokened[1]))
                        break;
                strcpy(textarr[textlen].addr, itob((0x400000 + loop[l].addr * 4) >> 2, 26));
                break;

            case 4: //s    jr
                strcpy(textarr[textlen].s, itob(atoi(tokened[1]), 5));
                strcpy(textarr[textlen].func, instruction[i].funct);
                break;

            case 5: //t, imm   lui
                strcpy(textarr[textlen].t, itob(atoi(tokened[1]), 5));
                if (tokened[2][1] == 'x')
                    strcpy(textarr[textlen].imm, itob(strtol(tokened[2], NULL, 16), 16));
                else
                    strcpy(textarr[textlen].imm, itob(atoi(tokened[2]), 16));
                break;

            case 6: //d, t, shamt
                strcpy(textarr[textlen].d, itob(atoi(tokened[1]), 5));
                strcpy(textarr[textlen].t, itob(atoi(tokened[2]), 5));
                if (tokened[3][1] == 'x')
                    strcpy(textarr[textlen].shamt, itob(strtol(tokened[3], NULL, 16), 5));
                else
                    strcpy(textarr[textlen].shamt, itob(atoi(tokened[3]), 5));
                strcpy(textarr[textlen].func, instruction[i].funct);
                break;

            case 7: //la
                for (d = 1; d <= datalen; d++)
                {
                    if (!strcmp(dataarr[d].name, tokened[2]))
                    {
                        if (dataarr[d].addr != 0)
                        {
                            flag = 1;
                            break;
                        }
                    }
                }
                strcpy(textarr[textlen].op, instruction[9].op);             //lui = instruction[9]에 저장되어있음.
                textarr[textlen].mode = instruction[9].mode - '0';
                strcpy(textarr[textlen].t, itob(atoi(tokened[1]), 5));
                strcpy(textarr[textlen].imm, itob(0x1000, 16));

                if (flag)
                {
                    for (l = 1; l <= loopcur; l++)
                        if (loop[l].addr >= textlen)
                            loop[l].addr++;
                    textlen++;
                    textarr[textlen].mode = instruction[14].mode - '0';
                    strcpy(textarr[textlen].op, instruction[14].op);        //ori = instruction[14]에 저장되어있음.
                    strcpy(textarr[textlen].s, itob(atoi(tokened[1]), 5));
                    strcpy(textarr[textlen].t, itob(atoi(tokened[1]), 5));
                    strcpy(textarr[textlen].imm, itob(dataarr[d].addr * 4, 16));
                }
                break;

            case 8: // lw sw
                strcpy(textarr[textlen].s, itob(atoi(tokened[3]), 5));
                strcpy(textarr[textlen].imm, itob(atoi(tokened[2]), 16));
                strcpy(textarr[textlen].t, itob(atoi(tokened[1]), 5));
                break;
            }
        }
    }
    fprintf(output, "%s", itob(textlen * 4, 32));
    fprintf(output, "%s", itob(datalen * 4 + 4, 32));

    //{d,s,t} == 0,      {t s imm} == 1,  {t s addr} == 2
    //{addr} == 3,       {s} == 4,        {t imm} == 5
    //{d t shamt} == 6,  {la} == 7

    for (int i = 0; i < textlen; i++)
    {
        switch (textarr[i].mode)
        {
        case 0: //R형식
        case 4:
        case 6:
            fprintf(output, "%s%s%s%s%s%s", textarr[i].op, textarr[i].s, textarr[i].t, textarr[i].d, textarr[i].shamt, textarr[i].func);
            break;

        case 1: //I형식
        case 2:
        case 5:
        case 8:
            fprintf(output, "%s%s%s%s", textarr[i].op, textarr[i].s, textarr[i].t, textarr[i].imm);
            break;

        case 3: //J형식
            fprintf(output, "%s%s", textarr[i].op, textarr[i].addr);
            break;
        }
    }
    for (int i = 0; i <= datalen; i++)
        fprintf(output, "%s", itob(dataarr[i].data, 32));
    fprintf(output, "\n");

    fclose(input);
    fclose(output);
    exit(EXIT_SUCCESS);
}

/*******************************************************
 * Function: change_file_ext
 *
 * Parameters:
 *  char
 *      *str: a raw filename (without path)
 *
 * Return:
 *  return NULL if a file is not an assembly file.
 *  return empty string
 *
 * Info:
 *  This function reads filename and converst it into
 *  object extention name, *.o
 *
 *******************************************************/
char *change_file_ext(char *str)
{
    char *dot = strrchr(str, '.');

    if (!dot || dot == str || (strcmp(dot, ".s") != 0))
    {
        return NULL;
    }

    str[strlen(str) - 1] = 'o';
    return "";
}

char *itob(int k, int j)
{ //정수를 바이너리로 변환 음수 지원
    int giho = 0, t = 0;
    if (k < 0)
    {
        k = -k;
        k = k - 1;
        giho = 1;
    }
    char *c = (char *)malloc(sizeof(char) * (j + 1));
    c[j--] = '\0';
    for (int i = 0; i <= j; i++)
        c[i] = '0';
    if (k == 0)
        return c;
    while (k)
    {
        c[j--] = (k % 2) + '0';
        k = k >> 1;
        t++;
    }
    if (giho == 1)
    {
        for (int i = 0; i <= j + t; i++)
        {
            if (c[i] == '0')
                c[i] = '1';
            else
                c[i] = '0';
        }
    }
    return c;
}

char **strtokenize(char s[])
{ //문자열을 잘라줌.
    char **dest, *src = malloc(sizeof(char) * 30);
    strcpy(src, s);
    dest = (char **)malloc(sizeof(char *) * 6);
    int c = 0;
    char *cur = strtok(src, "():$\n\t, ");
    dest[c] = cur;
    while (cur != NULL)
    {
        cur = strtok(NULL, "():$\n\t, ");
        dest[++c] = cur;
    }
    return dest;
}