#ifndef TOKEN_BUCKET_H
#define TOKEN_BUCKET_H

#include <mutex>

enum TokenType
{
    eum_token_green = 1,
    eum_token_yellow,
    eum_token_red,
    eum_token_end
};

enum Size
{
    eum_KB = 1024,
    eum_MB = 1024 * 1024,
    eum_GB = 1024 * 1024 * 1024
};

class TokenBucket
{
public:
    TokenBucket(size_t cbs, size_t ebs, size_t cir)
        : m_cbs(cbs)
        , m_ebs(ebs)
        , m_tc(cbs)
        , m_te(ebs)
        , m_cir(cir)
        , m_last_time(0)
    {
    }

    void SetCBS(size_t cbs)
    {
        std::lock_guard<std::mutex> lk(m_mut);
        m_cbs = cbs;
    }

    void SetEBS(size_t ebs)
    {
        std::lock_guard<std::mutex> lk(m_mut);
        m_ebs = ebs;
    }

    void SetCIR(size_t cir)
    {
        std::lock_guard<std::mutex> lk(m_mut);
        m_cir = cir;
    }

    bool Push(size_t data_size)
    {
        std::lock_guard<std::mutex> lk(m_mut);
        GenerateToken();
        TokenType type = GetType(data_size);
        bool ret = true;
        switch (type) {
            case eum_token_green:
                m_tc -= data_size;
                break;
            case eum_token_yellow:
                m_te -= data_size;
                break;
            case eum_token_red:
            default:
                ret = false;
        }
        return ret;
    }

private:
    TokenType GetType(size_t data_size) const
    {
        if (data_size <= m_tc)
        {
            return eum_token_green;
        }
        if (data_size > m_tc && data_size <= m_te)
        {
            return eum_token_yellow;
        }
        if (data_size > m_tc && data_size > m_te)
        {
            return eum_token_red;
        }
        return eum_token_end;
    }

    void GenerateToken()
    {
        int64_t cur = ::time(nullptr);
        if (!m_last_time)
        {
            m_last_time = cur;
            return;
        }

        if (cur - m_last_time <= 0)
        {
            return;
        }

        size_t left =  (cur - m_last_time) * m_cir;
        if (left > m_cbs + m_ebs)
        {
            m_tc = m_cbs;
            m_te = m_ebs;
            return;
        }

        size_t token_count = left + m_tc;
        if (token_count > m_cbs)
        {
            m_tc = m_cbs;
        }
        size_t te = token_count - m_tc + m_te;
        m_te = te > m_ebs ? m_ebs : te;
    }

    size_t m_cbs;
    size_t m_ebs;

    size_t m_tc;
    size_t m_te;

    size_t m_cir;

    std::mutex m_mut;
    int64_t m_last_time;
};

#endif //TOKEN_BUCKET_H

