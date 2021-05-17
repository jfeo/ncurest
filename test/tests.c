#include <check.h>
#include <stdlib.h>

#include "../src/http.h"

START_TEST(test_http_dump_request_no_headers) {
  http_request req;
  char *dump;

  req.method = "GET";
  req.uri = "http://example.com";
  req.header_count = 0;

  http_dump_request(req, &dump);
  ck_assert_str_eq(dump, "GET http://example.com HTTP/1.1\r\n\r\n");
}
END_TEST

START_TEST(test_http_dump_request_single_header) {
  http_request req;
  http_header hdr;
  char *dump;

  hdr.header = "Content-Length";
  hdr.value = "1234";

  req.method = "GET";
  req.uri = "http://test.net";
  req.header_count = 1;
  req.headers = &hdr;

  http_dump_request(req, &dump);
  ck_assert_str_eq(dump, "GET http://test.net HTTP/1.1\r\n"
                         "Content-Length: 1234\r\n\r\n");
}
END_TEST

START_TEST(test_http_dump_request_multiple_headers) {
  http_request req;
  http_header hdr[5];
  char *dump;

  hdr[0].header = "Content-Length";
  hdr[0].value = "1234";

  hdr[1].header = "Content-Type";
  hdr[1].value = "application/json";

  hdr[2].header = "User-Agent";
  hdr[2].value = "check-unit-testing-agent";

  hdr[3].header = "Cache-Control";
  hdr[3].value = "no-cache";

  hdr[4].header = "Connection";
  hdr[4].value = "close";

  req.method = "GET";
  req.uri = "http://test.net";
  req.header_count = 5;
  req.headers = hdr;

  http_dump_request(req, &dump);
  ck_assert_str_eq(dump,
                   "GET http://test.net HTTP/1.1\r\nContent-Length: "
                   "1234\r\nContent-Type: application/json\r\nUser-Agent: "
                   "check-unit-testing-agent\r\nCache-Control: "
                   "no-cache\r\nConnection: close\r\n\r\n");
}
END_TEST

START_TEST(test_http_parse_response_noheader) {
  http_response *resp;
  char *raw = ("HTTP/1.1 200 Okay\r\n"
               "\r\n");

  http_parse_response(raw, 21, &resp);

  ck_assert_ptr_nonnull(resp);
  ck_assert_str_eq(resp->http_version, "HTTP/1.1");
  ck_assert_int_eq(resp->status_code, 200);
  ck_assert_str_eq(resp->reason_phrase, "Okay");
  ck_assert_int_eq(resp->header_count, 0);
  ck_assert_int_eq(resp->content_length, 0);
  ck_assert_ptr_null(resp->body);
}
END_TEST

START_TEST(test_http_parse_response_single_header) {
  http_response *resp;
  char *raw = ("HTTP/1.1 200 Okay\r\n"
               "Server: http://server.com\r\n"
               "\r\n");

  http_parse_response(raw, 48, &resp);

  ck_assert_ptr_nonnull(resp);
  ck_assert_str_eq(resp->http_version, "HTTP/1.1");
  ck_assert_int_eq(resp->status_code, 200);
  ck_assert_str_eq(resp->reason_phrase, "Okay");
  ck_assert_int_eq(resp->header_count, 1);
  ck_assert_str_eq(resp->headers[0].header, "Server");
  ck_assert_str_eq(resp->headers[0].value, "http://server.com");
  ck_assert_int_eq(resp->content_length, 0);
  ck_assert_ptr_null(resp->body);
}
END_TEST

START_TEST(test_http_parse_response_multi_header) {
  http_response *resp;
  char *raw = ("HTTP/1.1 200 Okay\r\n"
               "Server: http://server.com\r\n"
               "Content-Length: 12\r\n"
               "Content-Type: application/json\r\n"
               "\r\n"
               "{\"prop\": 10}");

  http_parse_response(raw, 113, &resp);

  ck_assert_ptr_nonnull(resp);
  ck_assert_str_eq(resp->http_version, "HTTP/1.1");
  ck_assert_int_eq(resp->status_code, 200);
  ck_assert_str_eq(resp->reason_phrase, "Okay");
  ck_assert_int_eq(resp->header_count, 3);
  ck_assert_str_eq(resp->headers[0].header, "Server");
  ck_assert_str_eq(resp->headers[0].value, "http://server.com");
  ck_assert_str_eq(resp->headers[1].header, "Content-Length");
  ck_assert_str_eq(resp->headers[1].value, "12");
  ck_assert_str_eq(resp->headers[2].header, "Content-Type");
  ck_assert_str_eq(resp->headers[2].value, "application/json");
  ck_assert_int_eq(resp->content_length, 12);
  ck_assert_ptr_nonnull(resp->body);
  ck_assert_str_eq(resp->body, "{\"prop\": 10}");
}
END_TEST

START_TEST(test_http_parse_response_body) {
  http_response *resp;
  char *raw = ("HTTP/1.1 200 Okay\r\n"
               "Server: http://server.com\r\n"
               "\r\n"
               "body data");

  http_parse_response(raw, 57, &resp);

  ck_assert_ptr_nonnull(resp);
  ck_assert_ptr_nonnull(resp->body);
  ck_assert_ptr_eq(resp->body, &raw[48]);
}
END_TEST

Suite *http_suite(void) {
  Suite *s;
  TCase *tc_core;

  s = suite_create("http");

  tc_core = tcase_create("Core");

  tcase_add_test(tc_core, test_http_dump_request_no_headers);
  tcase_add_test(tc_core, test_http_dump_request_single_header);
  tcase_add_test(tc_core, test_http_dump_request_multiple_headers);
  tcase_add_test(tc_core, test_http_parse_response_noheader);
  tcase_add_test(tc_core, test_http_parse_response_single_header);
  tcase_add_test(tc_core, test_http_parse_response_multi_header);
  tcase_add_test(tc_core, test_http_parse_response_body);
  suite_add_tcase(s, tc_core);

  return s;
}

int main(int argc, char **argv) {
  int number_failed;
  Suite *s;
  SRunner *sr;

  s = http_suite();
  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
