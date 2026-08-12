// driver cấp thấp để xử lý chuỗi Intel string hex nhận được từ server


enum 
{
    Check_some_ok,
    Check_some_error,
    END_FILE,
    DATA_RECORD
};


uint8_t check_some(uint8_t *buff, uint8_t len)
{
    uint8_t sum =0;
    for (uint8_t i =0; i<len -1 ; i++) // lay het tru byte check some o cuoi ra
        {
            sum += buff[i];
        }
    
    sum = ~sum;
    sum += 1;
    if(sum == buff[len-1])
    {
        return Check_some_ok;
    }
    
    return Check_some_error;
}
int8_t char_to_byte (char c) // dùng để chuyển đổi giá trị string về giá trị thập phân dựa trên bảng mã ASCII
{
    if (c>= '0' && c<= '9'){ return (c -'0');}
    
    else if (c>= 'A' && c<= 'F'){ return (c -'A' + 10);}
    
    else if (c>= 'a' && c<= 'f'){ return (c -'a' + 10);}
    
    return -1;
}


void convert_string_intel_hex_to_array_hex (char *input, uint8_t *output)  // hàm này bỏ qua kí tự đầu Start đầu tiên của file Intel Hex và lấy 2 kí tự liền nhau ghép thành một byte
{
    uint8_t index = 0;
    if (*input == ':')
        {
            input++; 
            while (*input != '\0')
            {
                uint8_t hex_value = char_to_byte (*input)<<4; // lay 4 bit cao
                input ++;
                hex_value |= char_to_byte (*input); // lay 4 bit thap
                input ++;
                output [index ++] = hex_value; 
            }
        }
}

void swap(uint8_t *a, uint8_t *b)
{
    uint8_t temp = *a;
    *a = *b;
    *b = temp;
}
void swap_4_byte (uint8_t *data, uint8_t len) // hàm này không dùng
{
    for (uint8_t i =0; i< len; i+= 4)
    {
        swap (&data[i + 0], &data[i + 3]);
        swap (&data[i + 1], &data[i + 2]);
    }
}
