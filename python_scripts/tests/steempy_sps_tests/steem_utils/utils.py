def get_date_as_isostr(date):
    return date.replace(microsecond=0).isoformat()


def get_isostr_start_end_date(now, start_date_delta, end_date_delta):
    from datetime import timedelta

    start_date = now + timedelta(days = start_date_delta)
    end_date = start_date + timedelta(days = end_date_delta)

    start_date = start_date.replace(microsecond=0).isoformat()
    end_date = end_date.replace(microsecond=0).isoformat()

    return start_date, end_date